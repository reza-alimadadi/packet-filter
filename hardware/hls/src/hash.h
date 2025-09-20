#ifndef _HASH_H_
#define _HASH_H_

class ToeplitzHash {
private:
    static const int HASH_TABLE_SIZE = 32;
    /* Number of bits needed to index into the hash table.
     * This is calculated as the number of bits in HASH_TABLE_SIZE minus
     * the number of leading zeros in HASH_TABLE_SIZE, minus one.
     * This ensures we have enough bits to cover all indices in the table. */
    static constexpr int HASH_BITS = sizeof(HASH_TABLE_SIZE) * 8 - 1 -
                                     __builtin_clz(HASH_TABLE_SIZE);
    static constexpr uint32_t HASH_MASK = (1ULL << HASH_BITS) - 1;

    ap_uint<320> toeplitz_key;
    ap_uint<32> table[HASH_TABLE_SIZE];

    ap_uint<32> get_window(ap_uint<1> bit, int offset) {
        return bit ? toeplitz_key.range(offset + 31, offset) : 0;
    }
    ap_uint<32> compute_hash(ap_uint<32> ip, ap_uint<16> port);

public:
    ToeplitzHash();
    ap_uint<32> lookup(ap_uint<32> ip, ap_uint<16> port);
    void insert(ap_uint<32> ip, ap_uint<16> port, ap_uint<32> value);
};

#endif // _HASH_H_
