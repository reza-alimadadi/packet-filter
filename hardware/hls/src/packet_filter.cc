
#include <stdint.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>

#include "network.h"
#include "hash.h"

using axis_250_t = ap_axiu<512, 48, 0, 0>;

void process_packet(hls::stream<axis_250_t> &s_axis,
                    hls::stream<axis_250_t> &m_axis,
                    ap_uint<32> ipv4_addr,
                    ap_uint<16> udp_port,
                    ap_uint<8>  action);

void packet_filter(hls::stream<axis_250_t> &s_axis,
                   hls::stream<axis_250_t> &m_axis,

                   ap_uint<32> ipv4_addr,
                   ap_uint<16> udp_port,
                   ap_uint<8>  action // 0: drop, 1: forward
                   ) {
#pragma HLS INTERFACE axis          port=s_axis
#pragma HLS INTERFACE axis          port=m_axis
#pragma HLS INTERFACE s_axilite     port=ipv4_addr bundle=control clock=axil_aclk
#pragma HLS INTERFACE s_axilite     port=udp_port  bundle=control
#pragma HLS INTERFACE s_axilite     port=action    bundle=control
#pragma HLS INTERFACE ap_ctrl_none  port=return

#pragma HLS STABLE    variable=ipv4_addr
#pragma HLS STABLE    variable=udp_port
#pragma HLS STABLE    variable=action

    process_packet(s_axis, m_axis, ipv4_addr, udp_port, action);
}

void process_packet(hls::stream<axis_250_t> &s_axis,
                    hls::stream<axis_250_t> &m_axis,
                    ap_uint<32> ipv4_addr,
                    ap_uint<16> udp_port,
                    ap_uint<8>  action) {
#pragma HLS pipeline II=1 style=frp

    static ToeplitzHash hash_table;

    if (s_axis.empty()) {
        hash_table.insert(ipv4_addr, udp_port, action);
        return;
    }

    static int phit_idx = 0;
    static NetworkPacket network;

    axis_250_t incoming_phit;
    s_axis >> incoming_phit;

    network.deserialize(incoming_phit.data, phit_idx);

    ap_uint<32> table_action = 0;
    if (phit_idx == 0 && network.eth_hdr.is_ipv4() && network.ip_hdr.is_udp()) {
        table_action = hash_table.lookup(network.ip_hdr.src_ip, network.udp_hdr.src_port);
    }

    if (table_action == 1) { // forward
        axis_250_t outgoing_phit;
        outgoing_phit = {
            .data = incoming_phit.data,
            .keep = incoming_phit.keep,
            .user = incoming_phit.user,
            .last = incoming_phit.last
        };
        m_axis << outgoing_phit;
    }

    phit_idx = incoming_phit.last ? 0 : phit_idx + 1;
}
