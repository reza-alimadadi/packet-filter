#include <signal.h>
#include <unistd.h>
#include <sstream>

#include "deps.h"
#include "dpdk.h"
#include "packet_filter.h"

struct Arguments {
    const char* dpdk_config = nullptr;
    uint16_t num_threads = 1;
    uint32_t duration = 10;

    /* Filter format: <ipv4_addr>:<port>,... */
    std::vector<std::string> filter_list;

    void parse_args(int argc, const char** argv);
};

class Timeout {
private:
    std::mutex mutex;
    std::condition_variable cv;
    bool terminate = false;

public:
    void wait_for(uint64_t duration) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait_for(lock, std::chrono::seconds(duration), [this] { return terminate; });
    }

    void force_stop() {
        std::lock_guard<std::mutex> lock(mutex);
        terminate = true;
        cv.notify_all();
    }
};

int network_packet_handler(uint16_t thread_id, rte_mbuf* mbuf);

Timeout timeout;
void signal_handler(int signum) {
    log_info("Received signal %d, terminating...", signum);
    timeout.force_stop();
}

int main(int argc, const char** argv) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    Arguments args;
    args.parse_args(argc, argv);

    DPDK dpdk(args.dpdk_config, args.num_threads);
    for (uint16_t i = 0; i < args.num_threads; i++) {
        dpdk.register_callback(i, network_packet_handler);
    }
    PacketFilter packet_filter(args.filter_list);

    log_info("Running for %u seconds...", args.duration);
    timeout.wait_for(args.duration);
    log_info("Time's up, shutting down...");

    PacketAdapter packet_adapter;
    packet_adapter.show_stats();
    return 0;
}

void Arguments::parse_args(int argc, const char** argv) {
    int c;
    while ((c = getopt(argc, const_cast<char**>(argv), "c:t:d:f:")) != -1) {
        switch (c) {
            case 'c':
                this->dpdk_config = optarg;
                break;

            case 't':
                this->num_threads = static_cast<uint16_t>(std::stoi(optarg));
                break;

            case 'd':
                this->duration = static_cast<uint32_t>(std::stoi(optarg));
                break;

            case 'f': {
                std::string filter_str(optarg);
                std::stringstream ss(filter_str);
                std::string token;
                while (std::getline(ss, token, ',')) {
                    this->filter_list.push_back(token);
                }
                break;
            }

            case '?':
            default:
                log_info("Usage: %s -c <dpdk_config> -t <num_threads> -d <duration> -f <filter_list>", argv[0]);
                log_fatal("Unknown option: %c", c);
        }
    }

    if (this->dpdk_config == nullptr) {
        log_fatal("DPDK configuration string is required. Use -c option.");
    }
}

int network_packet_handler(uint16_t thread_id, rte_mbuf* mbuf) {
    log_assert(mbuf != nullptr, "Received null mbuf in packet handler");
    size_t buffer_offset = 0;
    size_t pkt_len = rte_pktmbuf_pkt_len(mbuf);

    /* Parse Ethernet header */
    rte_ether_hdr* eth_hdr = rte_pktmbuf_mtod_offset(mbuf, rte_ether_hdr*, buffer_offset);
    if (eth_hdr == nullptr) {
        log_warn("Failed to get Ethernet header");
        return -1;
    }
    if (rte_be_to_cpu_16(eth_hdr->ether_type) != RTE_ETHER_TYPE_IPV4) {
        log_debug("Non-IPv4 packet received, skipping");
        return 0;
    }

    buffer_offset += sizeof(rte_ether_hdr);

    /* Parse IPv4 header */
    rte_ipv4_hdr* ip_hdr = rte_pktmbuf_mtod_offset(mbuf, rte_ipv4_hdr*, buffer_offset);
    if (ip_hdr == nullptr) {
        log_warn("Failed to get IPv4 header");
        return -1;
    }
    if (ip_hdr->next_proto_id != IPPROTO_UDP) {
        log_debug("Non-UDP packet received, skipping");
        return 0;
    }

    buffer_offset += sizeof(rte_ipv4_hdr);

    /* Parse UDP header */
    rte_udp_hdr* udp_hdr = rte_pktmbuf_mtod_offset(mbuf, rte_udp_hdr*, buffer_offset);
    if (udp_hdr == nullptr) {
        log_warn("Failed to get UDP header");
        return -1;
    }

    /* Size sanity check */
    size_t udp_payload_len = rte_be_to_cpu_16(udp_hdr->dgram_len);
    if (buffer_offset + udp_payload_len > pkt_len) {
        log_error("UDP payload length exceeds packet length %zu > %zu",
                  buffer_offset + udp_payload_len, pkt_len);
        return -1;
    }

    buffer_offset += sizeof(rte_udp_hdr);
    uint8_t* udp_payload = rte_pktmbuf_mtod_offset(mbuf, uint8_t*, buffer_offset);
    if (udp_payload == nullptr) {
        log_warn("Failed to get UDP payload");
        return -1;
    }
    size_t payload_len = udp_payload_len - sizeof(rte_udp_hdr);

    /* Helper functions to convert binary data to string */
    auto convert_mac_to_str = [](uint8_t* mac) {
        char mac_str[18];
        sprintf(mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return std::string(mac_str);
    };
    auto convert_ip_to_str = [](uint32_t ipv4) {
        char ip_str[16];
        sprintf(ip_str, "%d.%d.%d.%d",
                ipv4 & 0xFF, (ipv4 >> 8) & 0xFF, (ipv4 >> 16) & 0xFF, (ipv4 >> 24) & 0xFF);
        return std::string(ip_str);
    };
    auto convert_bin_to_str = [](uint8_t* data, size_t len) {
        std::string str;
        for (size_t i = 0; i < len; i++) {
            char byte_str[3];
            snprintf(byte_str, sizeof(byte_str), "%02x", data[i]);
            str += byte_str;
        }
        return str;
    };

    /* Packet information logging */
    log_info("Received UDP packet on thread_id %u", thread_id);
    log_info("  ether_hdr: src=%s, dst=%s, ether_type=%x",
                convert_mac_to_str(eth_hdr->src_addr.addr_bytes).c_str(),
                convert_mac_to_str(eth_hdr->dst_addr.addr_bytes).c_str(),
                rte_be_to_cpu_16(eth_hdr->ether_type));
    log_info("  ipv4_hdr: src=%s, dst=%s, total_length=%d, next_proto_id=%x",
                convert_ip_to_str(rte_be_to_cpu_32(ip_hdr->src_addr)).c_str(),
                convert_ip_to_str(rte_be_to_cpu_32(ip_hdr->dst_addr)).c_str(),
                rte_be_to_cpu_16(ip_hdr->total_length),
                ip_hdr->next_proto_id);
    log_info("  udp_hdr: src_port=%d, dst_port=%d, dgram_len=%d",
                rte_be_to_cpu_16(udp_hdr->src_port),
                rte_be_to_cpu_16(udp_hdr->dst_port),
                rte_be_to_cpu_16(udp_hdr->dgram_len));

    log_info("  udp_payload (%zu bytes): %s", payload_len, 
             convert_bin_to_str(udp_payload, payload_len).c_str());
    return 0;
}

