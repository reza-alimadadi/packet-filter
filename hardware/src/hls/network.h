#ifndef _NETWORK_H_
#define _NETWORK_H_

struct EthernetHeader {
    ap_uint<48> dest_mac;
    ap_uint<48> src_mac;
    ap_uint<16> eth_type;

    enum { /* Ethertype values */
        IPV4 = 0x0800,
        ARP  = 0x0806
    };

    void serialize(ap_uint<512> &data, const int phit_idx) const;
    void deserialize(const ap_uint<512> &data, const int phit_idx);
    int size() const { return 14; } // Size in bytes
    bool is_ipv4() const { return eth_type == __builtin_bswap16(IPV4); }
};

struct IPv4Header {
    ap_uint<4>  version;
    ap_uint<4>  ihl;
    ap_uint<8>  dscp_ecn;
    ap_uint<16> total_length;
    ap_uint<16> identification;
    ap_uint<3>  flags;
    ap_uint<13> fragment_offset;
    ap_uint<8>  ttl;
    ap_uint<8>  protocol;
    ap_uint<16> header_checksum;
    ap_uint<32> src_ip;
    ap_uint<32> dest_ip;

    enum { /* Protocol values */
        TCP  = 6,
        UDP  = 17
    };

    void serialize(ap_uint<512> &data, const int phit_idx) const;
    void deserialize(const ap_uint<512> &data, const int phit_idx);
    int size() const { return 20; } // Size in bytes
    bool is_udp() const { return protocol == UDP; }
};

struct UDPHeader {
    ap_uint<16> src_port;
    ap_uint<16> dest_port;
    ap_uint<16> length;
    ap_uint<16> checksum;

    void serialize(ap_uint<512> &data, const int phit_idx) const;
    void deserialize(const ap_uint<512> &data, const int phit_idx);
    int size() const { return 8; } // Size in bytes
};

struct NetworkPacket {
    EthernetHeader eth_hdr;
    IPv4Header     ip_hdr;
    UDPHeader      udp_hdr;

    void serialize(ap_uint<512> &data, const int phit_idx) const;
    void deserialize(const ap_uint<512> &data, const int phit_idx);
    int size() const { return eth_hdr.size() + ip_hdr.size() + udp_hdr.size(); }
};

#endif // _NETWORK_H_
