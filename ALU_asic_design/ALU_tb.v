 
`timescale 1ns / 1ps 
 
module add_tb; 
 
reg clock; 
  reg reset; 
  reg start_port; 
  reg [31:0] x; 
  reg [31:0] y; 
  reg [31:0] select; 
  wire done_port; 
  wire [31:0] return_port; 
 
  ALU uut ( 
    .clock(clock), 
    .reset(reset), 
    .start_port(start_port), 
    .x(x), 
    .y(y), 
    .select(select), 
    .done_port(done_port), 
    .return_port(return_port) 
  ); 
 
  always #5 clock = ~clock; 
 
  task run_test; 
    input [31:0] a; 
    input [31:0] b; 
    input [31:0] sel; 
    begin 
      x = a; 
      y = b; 
      select = sel; 
      @(posedge clock); 
      start_port = 1; 
 
      @(posedge clock); 
      start_port = 0; 
 
      wait(done_port); 
      @(posedge clock);  
 
      $display("Time=%0t | select=%0d | x=%0d | 
y=%0d | result=%0d", 
                $time, sel, a, b, return_port); 
 
      repeat(2) @(posedge clock); 
    end 
  endtask 
  initial begin 
    $dumpfile("simulation1.vcd"); 
    $dumpvars(0, add_tb); 
 
    clock = 0; 
    reset = 0; 
    start_port = 0; 
    x = 0; 
    y = 0; 
    select = 0; 
 
    #20; 
    reset = 1; 
 
    run_test(12, 3, 3'b000);   // ADD 
    run_test(12, 3, 3'b001);   // SUB 
    run_test(12,  3, 3'b010);   // MUL 
    run_test(12, 3, 3'b011);  // AND 
    run_test(12, 3, 3'b100);  // OR 
    run_test(12, 3, 3'b101);  // XOR 
    run_test(12,  3, 3'b110);   // LEFT SHIFT 
    run_test(12, 3, 3'b111);   // RIGHT SHIFT 
 
    #50; 
    $finish; 
  end 
 
endmodule 
