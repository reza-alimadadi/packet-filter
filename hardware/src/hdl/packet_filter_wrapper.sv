
`timescale 1ns/1ps
module packet_filter_wrapper #(
    parameter int FUNC_ID = 0
)(
    input          s_axil_awvalid,
    input   [31:0] s_axil_awaddr,
    output         s_axil_awready,
    input          s_axil_wvalid,
    input   [31:0] s_axil_wdata,
    output         s_axil_wready,
    output         s_axil_bvalid,
    output   [1:0] s_axil_bresp,
    input          s_axil_bready,
    input          s_axil_arvalid,
    input   [31:0] s_axil_araddr,
    output         s_axil_arready,
    output         s_axil_rvalid,
    output  [31:0] s_axil_rdata,
    output   [1:0] s_axil_rresp,
    input          s_axil_rready,

    input          s_axis_qdma_h2c_tvalid,
    input  [511:0] s_axis_qdma_h2c_tdata,
    input   [63:0] s_axis_qdma_h2c_tkeep,
    input          s_axis_qdma_h2c_tlast,
    input   [47:0] s_axis_qdma_h2c_tuser,
    output         s_axis_qdma_h2c_tready,

    input          s_axis_adap_rx_tvalid,
    input  [511:0] s_axis_adap_rx_tdata,
    input   [63:0] s_axis_adap_rx_tkeep,
    input          s_axis_adap_rx_tlast,
    input   [47:0] s_axis_adap_rx_tuser,
    output         s_axis_adap_rx_tready,

    output         m_axis_qdma_c2h_tvalid,
    output [511:0] m_axis_qdma_c2h_tdata,
    output  [63:0] m_axis_qdma_c2h_tkeep,
    output         m_axis_qdma_c2h_tlast,
    output  [47:0] m_axis_qdma_c2h_tuser,
    input          m_axis_qdma_c2h_tready,

    output         m_axis_adap_tx_tvalid,
    output [511:0] m_axis_adap_tx_tdata,
    output  [63:0] m_axis_adap_tx_tkeep,
    output         m_axis_adap_tx_tlast,
    output  [47:0] m_axis_adap_tx_tuser,
    input          m_axis_adap_tx_tready,

    input          mod_rstn,
    output         mod_rst_done,

    input          axil_aclk,
    input          axis_aclk
);

    logic axil_aresetn;

	// Reset is clocked by the 125MHz AXI-Lite clock
    generic_reset #(
      .NUM_INPUT_CLK  (1),
      .RESET_DURATION (100)
    ) reset_inst (
      .mod_rstn     (mod_rstn),
      .mod_rst_done (mod_rst_done),
      .clk          (axil_aclk),
      .rstn         (axil_aresetn)
    );

    assign m_axis_adap_tx_tvalid  = s_axis_qdma_h2c_tvalid;
    assign m_axis_adap_tx_tdata   = s_axis_qdma_h2c_tdata;
    assign m_axis_adap_tx_tkeep   = s_axis_qdma_h2c_tkeep;
    assign m_axis_adap_tx_tlast   = s_axis_qdma_h2c_tlast;
    assign m_axis_adap_tx_tuser   = s_axis_qdma_h2c_tuser;
    assign s_axis_qdma_h2c_tready = m_axis_adap_tx_tready;

    generate
        if (FUNC_ID == 0) begin 
            packet_filter_0 packet_filter_inst (
                .s_axi_cfg_AWVALID  (s_axil_awvalid),
                .s_axi_cfg_AWADDR   (s_axil_awaddr),
                .s_axi_cfg_AWREADY  (s_axil_awready),
                .s_axi_cfg_WVALID   (s_axil_wvalid),
                .s_axi_cfg_WDATA    (s_axil_wdata),
                .s_axi_cfg_WSTRB    (4'hF),
                .s_axi_cfg_WREADY   (s_axil_wready),
                .s_axi_cfg_BVALID   (s_axil_bvalid),
                .s_axi_cfg_BRESP    (s_axil_bresp),
                .s_axi_cfg_BREADY   (s_axil_bready),
                .s_axi_cfg_ARVALID  (s_axil_arvalid),
                .s_axi_cfg_ARADDR   (s_axil_araddr),
                .s_axi_cfg_ARREADY  (s_axil_arready),
                .s_axi_cfg_RVALID   (s_axil_rvalid),
                .s_axi_cfg_RDATA    (s_axil_rdata),
                .s_axi_cfg_RRESP    (s_axil_rresp),
                .s_axi_cfg_RREADY   (s_axil_rready),

                .s_axis_TVALID      (s_axis_adap_rx_tvalid),
                .s_axis_TDATA       (s_axis_adap_rx_tdata),
                .s_axis_TKEEP       (s_axis_adap_rx_tkeep),
                .s_axis_TLAST       (s_axis_adap_rx_tlast),
                .s_axis_TUSER       (s_axis_adap_rx_tuser),
                .s_axis_TREADY      (s_axis_adap_rx_tready),

                .m_axis_TVALID      (m_axis_qdma_c2h_tvalid),
                .m_axis_TDATA       (m_axis_qdma_c2h_tdata),
                .m_axis_TKEEP       (m_axis_qdma_c2h_tkeep),
                .m_axis_TLAST       (m_axis_qdma_c2h_tlast),
                .m_axis_TUSER       (m_axis_qdma_c2h_tuser),
                .m_axis_TREADY      (m_axis_qdma_c2h_tready),

                .ap_clk             (axis_aclk),
                .ap_rst_n           (axil_aresetn),

                .axil_aclk          (axil_aclk),
                .ap_rst_n_axil_aclk (axil_aresetn)
            );
        end else begin
            packet_filter_1 packet_filter_inst (
                .s_axi_cfg_AWVALID  (s_axil_awvalid),
                .s_axi_cfg_AWADDR   (s_axil_awaddr),
                .s_axi_cfg_AWREADY  (s_axil_awready),
                .s_axi_cfg_WVALID   (s_axil_wvalid),
                .s_axi_cfg_WDATA    (s_axil_wdata),
                .s_axi_cfg_WSTRB    (4'hF),
                .s_axi_cfg_WREADY   (s_axil_wready),
                .s_axi_cfg_BVALID   (s_axil_bvalid),
                .s_axi_cfg_BRESP    (s_axil_bresp),
                .s_axi_cfg_BREADY   (s_axil_bready),
                .s_axi_cfg_ARVALID  (s_axil_arvalid),
                .s_axi_cfg_ARADDR   (s_axil_araddr),
                .s_axi_cfg_ARREADY  (s_axil_arready),
                .s_axi_cfg_RVALID   (s_axil_rvalid),
                .s_axi_cfg_RDATA    (s_axil_rdata),
                .s_axi_cfg_RRESP    (s_axil_rresp),
                .s_axi_cfg_RREADY   (s_axil_rready),

                .s_axis_TVALID      (s_axis_qdma_h2c_tvalid),
                .s_axis_TDATA       (s_axis_qdma_h2c_tdata),
                .s_axis_TKEEP       (s_axis_qdma_h2c_tkeep),
                .s_axis_TLAST       (s_axis_qdma_h2c_tlast),
                .s_axis_TUSER       (s_axis_qdma_h2c_tuser),
                .s_axis_TREADY      (s_axis_qdma_h2c_tready),

                .m_axis_TVALID      (m_axis_adap_tx_tvalid),
                .m_axis_TDATA       (m_axis_adap_tx_tdata),
                .m_axis_TKEEP       (m_axis_adap_tx_tkeep),
                .m_axis_TLAST       (m_axis_adap_tx_tlast),
                .m_axis_TUSER       (m_axis_adap_tx_tuser),
                .m_axis_TREADY      (m_axis_adap_tx_tready),

                .ap_clk             (axis_aclk),
                .ap_rst_n           (axil_aresetn),

                .axil_aclk          (axil_aclk),
                .ap_rst_n_axil_aclk (axil_aresetn)
            );
        end
    endgenerate

endmodule
