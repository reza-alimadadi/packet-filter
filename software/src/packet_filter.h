#ifndef _PACKET_FILTER_H_
#define _PACKET_FILTER_H_

class PacketFilter {
private:
    class MMIO {
    private:
        uint32_t base_addr_;
        uint32_t port_id_;
        uint32_t bar_id_;

        void mmio_reg_write(uint32_t offset, uint32_t value);
        uint32_t mmio_reg_read(uint32_t offset);

    public:
        MMIO() = delete;
        MMIO(uint32_t port_id);
        ~MMIO() {}
        void set_base_addr(uint32_t addr) { base_addr_ = addr; }

        /* Support reading/writing integral types up to 64 bits */
        template<typename T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
        void write(uint32_t offset, T value) {
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
};

#endif // _PACKET_FILTER_H_
