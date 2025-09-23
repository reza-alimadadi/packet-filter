
#include <ap_int.h>
#include "network.h"

void EthernetHeader::serialize(ap_uint<512> &data, const int phit_idx) const {
    switch (phit_idx) {
        case 0:
            data.range(47, 0)   = dest_mac;
            data.range(95, 48)  = src_mac;
            data.range(111, 96) = eth_type;
            break;
        default:
            break;
    }
}

void EthernetHeader::deserialize(const ap_uint<512> &data, const int phit_idx) {
    switch (phit_idx) {
        case 0:
            dest_mac = data.range(47, 0);
            src_mac  = data.range(95, 48);
            eth_type = data.range(111, 96);
            break;
        default:
            break;
    }
}

void IPv4Header::serialize(ap_uint<512> &data, const int phit_idx) const {
    switch (phit_idx) {
        case 0:
            data.range(115, 112) = version;
            data.range(119, 116) = ihl;
            data.range(127, 120) = dscp_ecn;
            data.range(143, 128) = total_length;
            data.range(159, 144) = identification;
            data.range(162, 160) = flags;
            data.range(175, 163) = fragment_offset;
            data.range(183, 176) = ttl;
            data.range(191, 184) = protocol;
            data.range(207, 192) = header_checksum;
            data.range(239, 208) = src_ip;
            data.range(271, 240) = dest_ip;
            break;
        default:
            break;
    }
}

void IPv4Header::deserialize(const ap_uint<512> &data, const int phit_idx) {
    switch (phit_idx) {
        case 0:
            version         = data.range(115, 112);
            ihl             = data.range(119, 116);
            dscp_ecn        = data.range(127, 120);
            total_length    = data.range(143, 128);
            identification  = data.range(159, 144);
            flags           = data.range(162, 160);
            fragment_offset = data.range(175, 163);
            ttl             = data.range(183, 176);
            protocol        = data.range(191, 184);
            header_checksum = data.range(207, 192);
            src_ip          = data.range(239, 208);
            dest_ip         = data.range(271, 240);
            break;
        default:
            break;
    }
}

void UDPHeader::serialize(ap_uint<512> &data, const int phit_idx) const {
    switch (phit_idx) {
        case 0:
            data.range(287, 272) = src_port;
            data.range(303, 288) = dest_port;
            data.range(319, 304) = length;
            data.range(335, 320) = checksum;
            break;
        default:
            break;
    }
}

void UDPHeader::deserialize(const ap_uint<512> &data, const int phit_idx) {
    switch (phit_idx) {
        case 0:
            src_port  = data.range(287, 272);
            dest_port = data.range(303, 288);
            length    = data.range(319, 304);
            checksum  = data.range(335, 320);
            break;
        default:
            break;
    }
}

void NetworkPacket::serialize(ap_uint<512> &data, const int phit_idx) const {
    eth_hdr.serialize(data, phit_idx);
    ip_hdr.serialize(data, phit_idx);
    udp_hdr.serialize(data, phit_idx);
}

void NetworkPacket::deserialize(const ap_uint<512> &data, const int phit_idx) {
    eth_hdr.deserialize(data, phit_idx);
    ip_hdr.deserialize(data, phit_idx);
    udp_hdr.deserialize(data, phit_idx);
}
