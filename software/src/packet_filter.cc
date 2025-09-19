
#include <assert.h>

#include <rte_ethdev.h>
#include <rte_pmd_qdma.h>

#include <type_traits>

#include "deps.h"
#include "packet_filter.h"

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
