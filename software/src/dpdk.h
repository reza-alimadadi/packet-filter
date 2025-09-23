#ifndef _DPDK_H_
#define _DPDK_H_

#include <rte_ethdev.h>

class DPDK {
private:
    static const size_t DPDK_MAX_MBUFS          = 8192;
    static const size_t DPDK_MBUF_CACHE_SIZE    = 250;
    static const size_t DPDK_DESC_RING_SIZE     = 2048;
    static const size_t DPDK_BURST_SIZE         = 32;
    static const size_t DPDK_WRITEBACK_THRESH   = 64;
    static const size_t DPDK_PREFETCH_NUM       = 4;

    using rx_callback_t = std::function<int(uint16_t, rte_mbuf* mbuf)>;
    struct thread_info;

    uint16_t port_num_;
    std::vector<rte_mempool*> mbuf_pools_;

    /* Main thread to initialize DPDK and will be used to launch one of the rx threads */
    std::thread main_thread_;

    std::vector<std::shared_ptr<thread_info>> thread_infos_;
    volatile bool force_quit_;


    /* Synchronization for DPDK initialization */
    std::mutex init_mutex_;
    std::condition_variable init_cv_;

private:
    int init_dpdk(const char* argv_str, int num_threads);
    int port_init(uint16_t port_id);

    static int dpdk_rx_loop(void* arg);
    void shutdown();

public:
    DPDK() = delete;
    DPDK(const char* dpdk_config, int num_threads = 1);
    ~DPDK();
    void register_callback(uint16_t thread_id, rx_callback_t rx_callback);
    void trigger_shutdown();

private:
    struct thread_info {
        uint16_t port_id;
        uint16_t queue_id;
        uint16_t thread_id;

        DPDK* dpdk_instance;
        rx_callback_t rx_callback;

        /* Synchronization for callback registration since the rx loop may start before
         * the callback is registered. */
        std::mutex callback_mutex;
        std::condition_variable callback_cv;

        thread_info(uint16_t port, uint16_t queue, uint16_t tid, DPDK* instance)
            : port_id(port), queue_id(queue), thread_id(tid), dpdk_instance(instance) {}
    };
};

#endif // _DPDK_H_
