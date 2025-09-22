
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
    input   [63:0] s_axis_qdma_h2c_keep,
    input          s_axis_qdma_h2c_tlast,
    input   [47:0] s_axis_qdma_h2c_tuser,
    output         s_axis_qdma_h2c_tready,

    input          s_axis_adap_rx_tvalid,
    input  [511:0] s_axis_adap_rx_tdata,
    input   [63:0] s_axis_adap_rx_keep,
    input          s_axis_adap_rx_tlast,
    input   [47:0] s_axis_adap_rx_tuser,
    output         s_axis_adap_rx_tready,

    output         m_axis_qdma_c2h_tvalid,
    output [511:0] m_axis_qdma_c2h_tdata,
    output  [63:0] m_axis_qdma_c2h_keep,
    output         m_axis_qdma_c2h_tlast,
    output  [47:0] m_axis_qdma_c2h_tuser,
    input          m_axis_qdma_c2h_tready,

    output         m_axis_adap_tx_tvalid,
    output [511:0] m_axis_adap_tx_tdata,
    output  [63:0] m_axis_adap_tx_keep,
    output         m_axis_adap_tx_tlast,
    output  [47:0] m_axis_adap_tx_tuser,
    input          m_axis_adap_tx_tready,

    input          mod_rstn,
    output         mod_rst_done,

    input          axil_aclk,
    input          axis_aclk
);

    logic axil_aresetn, axis_aresetn;

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
    assign m_axis_adap_tx_keep    = s_axis_qdma_h2c_keep;
    assign m_axis_adap_tx_tlast   = s_axis_qdma_h2c_tlast;
    assign m_axis_adap_tx_tuser   = s_axis_qdma_h2c_tuser;
    assign s_axis_qdma_h2c_tready = m_axis_adap_tx_tready;

    generate
        if (FUNC_ID == 0) begin 
            packet_filter_0 packet_filter_inst (
                .s_axi_cfg_awvalid  (s_axil_awvalid),
                .s_axi_cfg_awaddr   (s_axil_awaddr),
                .s_axi_cfg_awready  (s_axil_awready),
                .s_axi_cfg_wvalid   (s_axil_wvalid),
                .s_axi_cfg_wdata    (s_axil_wdata),
                .s_axi_cfg_wstrb    (4'hF),
                .s_axi_cfg_wready   (s_axil_wready),
                .s_axi_cfg_bvalid   (s_axil_bvalid),
                .s_axi_cfg_bresp    (s_axil_bresp),
                .s_axi_cfg_bready   (s_axil_bready),
                .s_axi_cfg_arvalid  (s_axil_arvalid),
                .s_axi_cfg_araddr   (s_axil_araddr),
                .s_axi_cfg_arready  (s_axil_arready),
                .s_axi_cfg_rvalid   (s_axil_rvalid),
                .s_axi_cfg_rdata    (s_axil_rdata),
                .s_axi_cfg_rresp    (s_axil_rresp),
                .s_axi_cfg_rready   (s_axil_rready),

                .s_axis_tvalid      (s_axis_adap_rx_tvalid),
                .s_axis_tdata       (s_axis_adap_rx_tdata),
                .s_axis_keep        (s_axis_adap_rx_keep),
                .s_axis_tlast       (s_axis_adap_rx_tlast),
                .s_axis_tuser       (s_axis_adap_rx_tuser),
                .s_axis_tready      (s_axis_adap_rx_tready),

                .m_axis_tvalid      (m_axis_qdma_c2h_tvalid),
                .m_axis_tdata       (m_axis_qdma_c2h_tdata),
                .m_axis_keep        (m_axis_qdma_c2h_keep),
                .m_axis_tlast       (m_axis_qdma_c2h_tlast),
                .m_axis_tuser       (m_axis_qdma_c2h_tuser),
                .m_axis_tready      (m_axis_qdma_c2h_tready),

                .ap_clk             (axis_aclk),
                .ap_rst_n           (axis_aresetn),

                .axil_aclk          (axil_aclk),
                .ap_rst_n_axil_aclk (axil_aresetn)
            );
        end else begin
            packet_filter_1 packet_filter_inst (
                .s_axi_cfg_awvalid  (s_axil_awvalid),
                .s_axi_cfg_awaddr   (s_axil_awaddr),
                .s_axi_cfg_awready  (s_axil_awready),
                .s_axi_cfg_wvalid   (s_axil_wvalid),
                .s_axi_cfg_wdata    (s_axil_wdata),
                .s_axi_cfg_wstrb    (4'hF),
                .s_axi_cfg_wready   (s_axil_wready),
                .s_axi_cfg_bvalid   (s_axil_bvalid),
                .s_axi_cfg_bresp    (s_axil_bresp),
                .s_axi_cfg_bready   (s_axil_bready),
                .s_axi_cfg_arvalid  (s_axil_arvalid),
                .s_axi_cfg_araddr   (s_axil_araddr),
                .s_axi_cfg_arready  (s_axil_arready),
                .s_axi_cfg_rvalid   (s_axil_rvalid),
                .s_axi_cfg_rdata    (s_axil_rdata),
                .s_axi_cfg_rresp    (s_axil_rresp),
                .s_axi_cfg_rready   (s_axil_rready),

                .s_axis_tvalid      (s_axis_qdma_h2c_tvalid),
                .s_axis_tdata       (s_axis_qdma_h2c_tdata),
                .s_axis_keep        (s_axis_qdma_h2c_keep),
                .s_axis_tlast       (s_axis_qdma_h2c_tlast),
                .s_axis_tuser       (s_axis_qdma_h2c_tuser),
                .s_axis_tready      (s_axis_qdma_h2c_tready),

                .m_axis_tvalid      (m_axis_adap_tx_tvalid),
                .m_axis_tdata       (m_axis_adap_tx_tdata),
                .m_axis_keep        (m_axis_adap_tx_keep),
                .m_axis_tlast       (m_axis_adap_tx_tlast),
                .m_axis_tuser       (m_axis_adap_tx_tuser),
                .m_axis_tready      (m_axis_adap_tx_tready),

                .ap_clk             (axis_aclk),
                .ap_rst_n           (axis_aresetn),

                .axil_aclk          (axil_aclk),
                .ap_rst_n_axil_aclk (axil_aresetn)
            );
        end
    endgenerate

endmodule
