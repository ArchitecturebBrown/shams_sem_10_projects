module sync_2ff #(parameter ADDR_WIDTH=4) (
    input clk, rst_n, 
    input [ADDR_WIDTH:0] data_in, 
   output reg [ADDR_WIDTH:0] data_out
  );
  reg [ADDR_WIDTH:0] intermediate_signal;
  always@(posedge clk) begin
    if(!rst_n) begin
      intermediate_signal <= 0;
      data_out <= 0;
    end
    else begin
      intermediate_signal <= data_in;
      data_out <= intermediate_signal;
    end
  end
endmodule