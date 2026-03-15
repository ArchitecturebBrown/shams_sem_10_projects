module async_fifo #(parameter DEPTH=16, ADDR_WIDTH = 4, DATA_WIDTH=8) (
  input wclk, wrst_n,
  input rclk, rrst_n,
  input w_en, r_en,
  input [DATA_WIDTH-1:0] data_in,
  output reg [DATA_WIDTH-1:0] data_out,
  output reg full, empty
);
  
  reg [ADDR_WIDTH:0] g_wptr_sync, g_rptr_sync;
  reg [ADDR_WIDTH:0] b_wptr, b_rptr;
  reg [ADDR_WIDTH:0] g_wptr, g_rptr;

  wire [ADDR_WIDTH-1:0] waddr, raddr;
  wire internal_w_en = w_en & !full;
  wire internal_r_en = r_en & !empty;
  assign waddr = b_wptr[ADDR_WIDTH-1:0];
  assign raddr = b_rptr[ADDR_WIDTH-1:0];

  sync_2ff #(ADDR_WIDTH) sync_wptr (.clk(rclk), .rst_n(rrst_n), .data_in(g_wptr), .data_out(g_wptr_sync)); //write pointer to read clock domain
  sync_2ff #(ADDR_WIDTH) sync_rptr (.clk(wclk), .rst_n(wrst_n), .data_in(g_rptr), .data_out(g_rptr_sync)); //read pointer to write clock domain 
  
  wr_ctrl #(ADDR_WIDTH) wptr_h(wclk, wrst_n, w_en, g_rptr_sync, b_wptr, g_wptr, full);
  rd_ctrl #(ADDR_WIDTH) rptr_h(rclk, rrst_n, r_en, g_wptr_sync, b_rptr, g_rptr, empty);
  fifo_mem fifom(.wclk(wclk), .w_en(internal_w_en), .rclk(rclk), .r_en(internal_r_en), .b_wptr(b_wptr), .b_rptr(b_rptr), .data_in(data_in), .full(full), .empty(empty), .data_out(data_out));



`ifdef FORMAL
  //assertions


always @(*) assume(wrst_n);
always @(*) assume(rrst_n);

// Let clocks toggle properly
reg f_past_valid = 0;
initial f_past_valid = 0;
always @(posedge wclk) f_past_valid <= 1;

always @(*) begin
    if (f_past_valid) begin
        assume(wclk != ~(wclk));
        assume(rclk != ~(rclk));
    end
end

  //assert to check if fifo write signal can be asserted if full is high
  always @ (*) begin
    if(wrst_n) begin
      if (w_en == 1) begin
        assert (!(internal_w_en && full));
        end
  end
  end

  //assert to check if fifo read signal can be enabled if empty is high

   always @ (*) begin
    if(rrst_n) begin
      if (r_en == 1) begin
        assert (!(internal_r_en && empty));
        end
  end
  end 
//assert to check if write pointer can exceed depth range
   always @ (*) begin
    if(wrst_n) begin
      
      assert (waddr < DEPTH);
      
  end
  end 

  assume property ( !(r_en && empty) );
  assume property ( !(w_en && full) );
  
//cover property full and empty
cover_fifo_fill: cover property (full);
cover_fifo_drain: cover property (empty);


`endif
endmodule