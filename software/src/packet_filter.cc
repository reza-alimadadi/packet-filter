
#include <assert.h>

#include <rte_ethdev.h>
#include <rte_pmd_qdma.h>

#include <type_traits>

#include "deps.h"
#include "packet_filter.h"

PacketFilter::PacketFilter(std::vector<std::string> filter_list) : mmio_(0) {
    mmio_.set_base_addr(OPENNIC_USER_250_BASE_ADDR + PACKET_FILTER_OFFSET);

    for (const auto& filter : filter_list) {
        update_rule(filter, RULE_ACTION_FORWARD);
    }
}

void PacketFilter::update_rule(std::string net_addr, RuleAction action) {
    log_info("Updating rule: %s -> %s",
             net_addr.c_str(),
             action == RULE_ACTION_DROP ? "DROP" : "FORWARD");

    /* Get IP and port from net_addr */
    size_t colon_pos = net_addr.find(':');
    log_assert(colon_pos != std::string::npos, "Invalid net_addr format: %s",
               net_addr.c_str());
    std::string ip_str = net_addr.substr(0, colon_pos);
    std::string port_str = net_addr.substr(colon_pos + 1);

    /* convert IPv4 to uint32_t and network byte order */
    uint32_t ip = inet_addr(ip_str.c_str());
    log_assert(ip != INADDR_NONE, "Invalid IP address: %s", ip_str.c_str());

    /* convert port to uint16_t and network byte order */
    uint16_t port = htons(static_cast<uint16_t>(std::stoi(port_str)));
    log_assert(port != 0, "Invalid port: %s", port_str.c_str());

    /* Write to MMIO registers */
    mmio_.write<uint32_t>(RegisterMap::IPV4_ADDR_REG, ip);
    mmio_.write<uint16_t>(RegisterMap::UDP_PORT_REG, port);
    mmio_.write<uint8_t>(RegisterMap::RULE_ACTION_REG, static_cast<uint8_t>(action));
}

/* DPDK does not provide APIs to read/write QDMA registers, but QDMA PMD
 * provides two functions to read/write registers through PCIe.
 * We declare them here to use in our MMIO class.
 */
extern "C" {
void qdma_pci_write_reg(struct rte_eth_dev *dev, uint32_t bar, uint32_t reg, uint32_t val);
uint32_t qdma_pci_read_reg(struct rte_eth_dev *dev, uint32_t bar, uint32_t reg);
}

PacketFilter::MMIO::MMIO(uint32_t port_id) : port_id_(port_id) {
    /* We need to configure DPDK before creating MMIO object */
    if (rte_pmd_qdma_get_device(port_id) == nullptr) {
        log_fatal("Port %u is not a QDMA device", port_id);
    }

    uint8_t port_num = rte_eth_dev_count_avail();
    log_assert(port_id < port_num, "Invalid port_id: %u", port_id);

    int32_t config_bar, user_bar, bypass_bar;
    int ret = rte_pmd_qdma_get_bar_details(port_id, &config_bar, &user_bar, &bypass_bar);
    if (ret != 0) {
        log_fatal("Failed to get BAR details for port %u: %s",
                  port_id, rte_strerror(-ret));
    }

    port_id_ = port_id;
    bar_id_ = static_cast<uint32_t>(user_bar);
}

void PacketFilter::MMIO::mmio_reg_write(uint32_t offset, uint32_t value) {
    log_assert(base_addr_ != 0, "Base address is not set");
    rte_pmd_qdma_compat_pci_write_reg(port_id_, bar_id_, base_addr_ + offset, value);
}

uint32_t PacketFilter::MMIO::mmio_reg_read(uint32_t offset) {
    log_assert(base_addr_ != 0, "Base address is not set");
    return rte_pmd_qdma_compat_pci_read_reg(port_id_, bar_id_, base_addr_ + offset);
}
