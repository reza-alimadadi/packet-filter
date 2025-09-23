#ifndef _PACKET_FILTER_H_
#define _PACKET_FILTER_H_

class PacketFilter {
private:
    static const uint32_t OPENNIC_USER_250_BASE_ADDR = 0x100000;
    static const uint32_t PACKET_FILTER_OFFSET       = 0x20000;

    /* Register offsets within the Packet Filter HLS core 
     * Found in the file bellow after synthesis of the core
     * hardware/hls/packet_filter/solution1/impl/ip/drivers/packet_filter_v1_0/src/xpacket_filter_hw.h
     */
    enum RegisterMap : uint32_t {
        IPV4_ADDR_REG      = 0x10, /* 32 bits */
        UDP_PORT_REG       = 0x18, /* 16 bits */
        RULE_ACTION_REG    = 0x20, /* 8 bits */
    };

    class MMIO {
    private:
        uint32_t base_addr_;
        uint32_t port_id_;
        uint32_t bar_id_;

        void mmio_reg_write(uint32_t offset, uint32_t value);
        uint32_t mmio_reg_read(uint32_t offset);

    public:
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


private:
    MMIO mmio_;

public:
    enum RuleAction : uint32_t {
        RULE_ACTION_DROP = 0,
        RULE_ACTION_FORWARD = 1,
    };

    PacketFilter() : mmio_(0) {
        mmio_.set_base_addr(OPENNIC_USER_250_BASE_ADDR + PACKET_FILTER_OFFSET);
    }
    PacketFilter(std::vector<std::string> filter_list);
    ~PacketFilter() {}

    void update_rule(std::string net_addr, RuleAction action);


private:
};

#endif // _PACKET_FILTER_H_
