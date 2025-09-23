
#include <rte_eal.h>
/* #include <rte_ethdev.h> */
#include <rte_pmd_qdma.h>

#include "deps.h"
#include "dpdk.h"

DPDK::DPDK(const char* dpdk_config, int num_threads) {
    force_quit_ = false;

    main_thread_ = std::thread([this, dpdk_config, num_threads]() {
        int ret = init_dpdk(dpdk_config, num_threads);
        if (ret < 0) {
            log_fatal("Failed to initialize DPDK");
        }
    });
    std::unique_lock<std::mutex> lock(init_mutex_);
    init_cv_.wait(lock);
}

DPDK::~DPDK() {
    if (!force_quit_) {
        trigger_shutdown();
    }
    shutdown();
}

void DPDK::trigger_shutdown() {
    force_quit_ = true;
}

void DPDK::register_callback(uint16_t thread_id, rx_callback_t rx_callback) {
    log_assert(thread_id < thread_infos_.size(), "Invalid thread_id: %u", thread_id);

    log_debug("Registering callback for thread_id: %u", thread_id);
    auto& tinfo = thread_infos_[thread_id];

    tinfo->rx_callback = rx_callback;
    {
        std::lock_guard<std::mutex> lock(tinfo->callback_mutex);
        tinfo->callback_cv.notify_all();
    }
}

void DPDK::shutdown() {
    main_thread_.join();
    rte_eal_mp_wait_lcore();

    for (uint16_t port_id = 0; port_id < port_num_; port_id++) {
        /* TODO: add dpdk stats print */
        rte_eth_dev_stop(port_id);
        rte_eth_dev_close(port_id);
    }

    rte_eal_cleanup();
}

int DPDK::dpdk_rx_loop(void* arg) {
    auto* tinfo = static_cast<thread_info*>(arg);
    DPDK* dpdk = tinfo->dpdk_instance;

    /* Make sure callback is registered before starting the rx loop 
     * to avoid dropping packets or processing packets without a callback. */
    {
        std::unique_lock<std::mutex> lock(tinfo->callback_mutex);
        tinfo->callback_cv.wait(lock, [tinfo, dpdk]() -> bool {
            if (dpdk->force_quit_) {
                return true;
            }
            log_debug("Waiting for callback registration on thread_id: %u",
                      tinfo->thread_id);
            return tinfo->rx_callback != nullptr;
        });
    }

    log_info("Starting rx loop on thread_id: %u, port_id: %u, queue_id: %u",
             tinfo->thread_id, tinfo->port_id, tinfo->queue_id);

    struct rte_mbuf* bufs[DPDK_BURST_SIZE];
    while (!dpdk->force_quit_) {
        uint16_t nb_rx = rte_eth_rx_burst(tinfo->port_id, tinfo->queue_id,
                                          bufs, DPDK_BURST_SIZE);
        if (nb_rx == 0) {
            continue;
        }

        log_debug("Received %u packets on thread_id: %u", nb_rx, tinfo->thread_id);

        /* Prefetch first packets to the cache for processing */
        for (uint16_t i = 0; i < nb_rx && i < DPDK_PREFETCH_NUM; i++) {
            rte_prefetch0(rte_pktmbuf_mtod(bufs[i], uint8_t*));
        }

        for (uint16_t i = 0; i < nb_rx; i++) {
            log_debug("Processing packet %u on thread_id %u with length %u",
                      i, tinfo->thread_id, rte_pktmbuf_pkt_len(bufs[i]));

            /* Process the packet using the registered callback */
            int ret = tinfo->rx_callback(tinfo->thread_id, bufs[i]);
            if (ret < 0) {
                log_warn("Packet processing failed on thread_id %u, packet %u",
                         tinfo->thread_id, i);
            }

            /* Free the mbuf after processing */
            rte_pktmbuf_free(bufs[i]);

            /* Prefetch next packets */
            if (i + DPDK_PREFETCH_NUM < nb_rx) {
                rte_prefetch0(rte_pktmbuf_mtod(bufs[i + DPDK_PREFETCH_NUM], uint8_t*));
            }
        }
    }

    return 0;
}

int DPDK::init_dpdk(const char* argv_str, int num_threads) {
    std::vector<const char*> dpdk_argv;
    char* token = strtok(const_cast<char*>(argv_str), " ");
    while (token != nullptr) {
        dpdk_argv.push_back(token);
        token = strtok(nullptr, " ");
    }

    int ret = rte_eal_init(dpdk_argv.size(), const_cast<char**>(dpdk_argv.data()));
    if (ret < 0) {
        log_fatal("Failed to initialize EAL");
        return -1;
    }

    port_num_ = rte_eth_dev_count_avail();
    if (port_num_ == 0) {
        log_fatal("No available Ethernet ports");
        return -1;
    }

    uint16_t adjusted_threads = num_threads;
    if (num_threads % port_num_) {
        adjusted_threads = ((num_threads / port_num_) + 1) * port_num_;
        log_warn("Number of threads (%d) is not a multiple of number of ports (%d). "
                 "Adjusting number of threads to %zu.",
                 num_threads, port_num_, adjusted_threads);
    }
    num_threads = adjusted_threads;

    mbuf_pools_.resize(num_threads);
    thread_infos_.resize(num_threads);

    for (int i = 0; i < num_threads; i++) {
        std::string pool_name = "MBUF_POOL_" + std::to_string(i);
        mbuf_pools_[i] = rte_pktmbuf_pool_create(pool_name.c_str(), DPDK_MAX_MBUFS,
                                                 DPDK_MBUF_CACHE_SIZE, 0,
                                                 RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
        if (mbuf_pools_[i] == nullptr) {
            log_fatal("Cannot create %s mbuf pool: %s",
                      pool_name.c_str(), rte_strerror(rte_errno));
            return -1;
        }
    }

    for (uint16_t port_id = 0; port_id < port_num_; port_id++) {
        if (port_init(port_id) != 0) {
            return -1;
        }

        size_t queue_num = num_threads / port_num_;
        for (size_t q = 0; q < queue_num; q++) {
            uint16_t thread_id = port_id * queue_num + q;
            thread_infos_[thread_id] = std::make_shared<thread_info>(port_id, q, thread_id, this);
        }
    }

    init_cv_.notify_all();

    /* Launch rx loops on all lcores except the main thread's lcore */
    for (uint16_t lcore_id = 1; lcore_id < thread_infos_.size(); lcore_id++) {
        ret = rte_eal_remote_launch(dpdk_rx_loop, thread_infos_[lcore_id].get(), lcore_id);
        if (ret != 0) {
            log_fatal("Failed to launch rx loop on lcore %u: %s", lcore_id, rte_strerror(-ret));
            return -1;
        }
    }

    /* Run rx loop on the main thread's lcore */
    return dpdk_rx_loop(thread_infos_[0].get());
}

int DPDK::port_init(uint16_t port_id) {
    if (!rte_eth_dev_is_valid_port(port_id)) {
        log_error("Invalid port id %u", port_id);
        return -1;
    }

    struct rte_eth_dev_info dev_info;
    int ret = rte_eth_dev_info_get(port_id, &dev_info);
    if (ret < 0) {
        log_error("Failed to get device info for port %u: %s",
                  port_id, rte_strerror(-ret));
        return -1;
    }

    struct rte_eth_conf port_conf;
    memset(&port_conf, 0, sizeof(port_conf));

    /* Since we are only receiving packets, we only need to configure RX queues */
    size_t queue_num = thread_infos_.size() / port_num_;
    ret = rte_eth_dev_configure(port_id, queue_num, 0, &port_conf);
    if (ret < 0) {
        log_error("Failed to configure port %u: %s", port_id, rte_strerror(-ret));
        return -1;
    }

    /* Adjust number of descriptors */
    uint16_t rx_rings = DPDK_DESC_RING_SIZE;
    uint16_t tx_rings = 0;
    ret = rte_eth_dev_adjust_nb_rx_tx_desc(port_id, &rx_rings, &tx_rings);
    if (ret < 0) {
        log_error("Failed to adjust number of descriptors for port %u: %s",
                  port_id, rte_strerror(-ret));
        return -1;
    }

    struct rte_eth_rxconf rxconf;
    memset(&rxconf, 0, sizeof(rxconf));
    rxconf = dev_info.default_rxconf;
    rxconf.rx_thresh.wthresh = DPDK_WRITEBACK_THRESH;

    for (size_t q = 0; q < queue_num; q++) {
        uint16_t thread_id = port_id * queue_num + q;
        ret = rte_eth_rx_queue_setup(port_id, q, rx_rings,
                                     rte_eth_dev_socket_id(port_id),
                                     &rxconf, mbuf_pools_[thread_id]);
        if (ret < 0) {
            log_error("Failed to setup RX queue %zu for port %u: %s",
                      q, port_id, rte_strerror(-ret));
            return -1;
        }
    }

    ret = rte_eth_dev_start(port_id);
    if (ret < 0) {
        log_error("Failed to start port %u: %s", port_id, rte_strerror(-ret));
        return -1;
    }

    ret = rte_eth_stats_reset(port_id);
    if (ret < 0) {
        log_error("Failed to reset stats for port %u: %s", port_id, rte_strerror(-ret));
        return -1;
    }

    return 0;
}

