#include <stdint.h>
#include <ap_int.h>
#include "hash.h"

ToeplitzHash::ToeplitzHash() {
    /* Initialize the Toeplitz key with a predefined 320-bit value.
     * This key is used in the hash computation.
     * The key is represented as an ap_uint<320> where each bit can be accessed. */
    toeplitz_key.range(31, 0)     = 0xD6E31417;
    toeplitz_key.range(63, 32)    = 0x376CC87E;
    toeplitz_key.range(95, 64)    = 0x011BA7A6;
    toeplitz_key.range(127, 96)   = 0xDC1B91BB;
    toeplitz_key.range(159, 128)  = 0x7872E224;
    toeplitz_key.range(191, 160)  = 0xBFD0404B;
    toeplitz_key.range(223, 192)  = 0x260374B8;
    toeplitz_key.range(255, 224)  = 0xD9270F6F;
    toeplitz_key.range(287, 256)  = 0x18DC4386;
    toeplitz_key.range(319, 288)  = 0x7C9C37DE;
}

ap_uint<32> ToeplitzHash::compute_hash(ap_uint<32> ip, ap_uint<16> port) {
#pragma HLS expression_balance
    ap_uint<32> hash = 0;
    for (int i = 0; i < 32; i++) {
#pragma HLS unroll
        hash ^= get_window(ip[i], i);
    }
    for (int i = 0; i < 16; i++) {
#pragma HLS unroll
        hash ^= get_window(port[i], i + 32);
    }
    return hash;
}

ap_uint<32> ToeplitzHash::lookup(ap_uint<32> ip, ap_uint<16> port) {
    ap_uint<32> hash = compute_hash(ip, port);
    int index = hash & HASH_MASK;
    return table[index];
}

void ToeplitzHash::insert(ap_uint<32> ip, ap_uint<16> port, ap_uint<32> value) {
    ap_uint<32> hash = compute_hash(ip, port);
    int index = hash & HASH_MASK;
    table[index] = value;
}
