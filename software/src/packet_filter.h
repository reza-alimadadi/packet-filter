#ifndef _PACKET_FILTER_H_
#define _PACKET_FILTER_H_

class MMIO {
protected:
    uint32_t base_addr_;
    uint32_t port_id_;
    uint32_t bar_id_;

    void mmio_reg_write(uint32_t offset, uint32_t value);
    uint32_t mmio_reg_read(uint32_t offset);

protected:
    MMIO() = delete;
    MMIO(uint32_t port_id = 0);
    ~MMIO() {}
    void set_base_addr(uint32_t addr) { base_addr_ = addr; }

    /* Support reading/writing integral types up to 64 bits */
    template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    void write(uint32_t offset, T value) {
        /* Assert that the size of T is supported (up to 64 bits) */
        static_assert(sizeof(T) <= sizeof(uint64_t), "Unsupported type size");

        if constexpr (sizeof(T) <= sizeof(uint32_t)) {
            mmio_reg_write(offset, static_cast<uint32_t>(value));
        }
        else {
            mmio_reg_write(offset, static_cast<uint32_t>(value & 0xFFFFFFFF));
            mmio_reg_write(offset + 4, static_cast<uint32_t>((value >> 32) & 0xFFFFFFFF));
        }
    }

    template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    T read(uint32_t offset) {
        static_assert(sizeof(T) <= sizeof(uint64_t), "Unsupported type size");

        if constexpr (sizeof(T) <= sizeof(uint32_t)) {
            return static_cast<T>(mmio_reg_read(offset));
        }
        else {
            uint64_t low = mmio_reg_read(offset);
            uint64_t high = mmio_reg_read(offset + 4);
            return static_cast<T>((high << 32) | low);
        }
    }
};


class PacketFilter : public MMIO {
private:
    static const uint32_t OPENNIC_USER_250_BASE_ADDR = 0x100000;
    static const uint32_t PACKET_FILTER_OFFSET       = 0x2000;

    /* Register offsets within the Packet Filter HLS core 
     * Found in the file bellow after synthesis of the core
     * hardware/hls/packet_filter/solution1/impl/ip/drivers/packet_filter_v1_0/src/xpacket_filter_hw.h
     */
    enum RegisterMap : uint32_t {
        IPV4_ADDR_REG       = 0x10, /* 32 bits */
        UDP_PORT_REG        = 0x18, /* 16 bits */
        RULE_ACTION_REG     = 0x20, /* 8 bits */

        STATS_PKT_IN_REG    = 0x28, /* 64 bits */
        STATS_PHIT_IN_REG   = 0x40, /* 64 bits */
        STATS_PKT_FORWD_REG = 0x58, /* 64 bits */
        STATS_PKT_DROP_REG  = 0x70, /* 64 bits */
    };

public:
    enum RuleAction : uint32_t {
        RULE_ACTION_DROP = 0,
        RULE_ACTION_FORWARD = 1,
    };

    PacketFilter() : MMIO(0) {
        set_base_addr(OPENNIC_USER_250_BASE_ADDR + PACKET_FILTER_OFFSET);
    }
    PacketFilter(std::vector<std::string> filter_list);
    ~PacketFilter() {}

    void update_rule(std::string net_addr, RuleAction action);
    void show_stats();
};

class PacketAdapter : public MMIO {
private:
    static const uint32_t OPENNIC_ADAP_BASE_ADDR = 0xB000;

    enum RegisterMap : uint32_t {
        TX_PACKET_SENT_REG    = 0x00, /* 64 bits */
        TX_PACKET_DROPPED_REG = 0x10, /* 64 bits */
        RX_PACKET_RECV_REG    = 0x20, /* 64 bits */
        RX_PACKET_DROPPED_REG = 0x30, /* 64 bits */
        RX_PACKET_ERROR_REG   = 0x40, /* 64 bits */
    };

public:
    PacketAdapter() : MMIO(0) {
        set_base_addr(OPENNIC_ADAP_BASE_ADDR);
    }
    ~PacketAdapter() {}
    void show_stats();
};

#endif // _PACKET_FILTER_H_
