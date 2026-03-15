module fifo_mem #(parameter DEPTH=8, DATA_WIDTH=8, ADDR_WIDTH=4) (
  input wclk, w_en, rclk, r_en,
  input [ADDR_WIDTH:0] b_wptr, b_rptr,
  input [DATA_WIDTH-1:0] data_in,
  input full, empty,
  output reg [DATA_WIDTH-1:0] data_out
);
  reg [DATA_WIDTH-1:0] fifo[0:DEPTH-1];
  
  always@(posedge wclk) begin
    if(w_en & !full) begin
      fifo[b_wptr[ADDR_WIDTH-1:0]] <= data_in;
    end
  end

  assign data_out = fifo[b_rptr[ADDR_WIDTH-1:0]];
endmodule