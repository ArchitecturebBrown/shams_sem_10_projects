// ==========================================================
// 2. Top-Level Divider - Verilog-2001
// ==========================================================
module nr_divider_top(
    input clk, rst,
    input signed [31:0] dividend, divisor,
    output signed [31:0] quotient
);
    reg signed [31:0] x_seed;
    reg [3:0] d_high;

    wire signed [31:0] a_bus0, a_bus1, a_bus2, a_bus3, a_bus4;
    wire signed [31:0] b_bus0, b_bus1, b_bus2, b_bus3, b_bus4;
    wire signed [31:0] x_bus0, x_bus1, x_bus2, x_bus3, x_bus4;

    assign a_bus0 = dividend;
    assign b_bus0 = divisor;

    // LUT for x0
    always @(*) begin
        d_high = divisor[19:16];
        case(d_high)
            4'h0: x_seed = 32'sh0001_0000; 4'h1: x_seed = 32'sh0000_FFFF;
            4'h2: x_seed = 32'sh0000_8000; 4'h3: x_seed = 32'sh0000_5555;
            4'h4: x_seed = 32'sh0000_4000; 4'h5: x_seed = 32'sh0000_3333;
            4'h6: x_seed = 32'sh0000_2AAA; 4'h7: x_seed = 32'sh0000_2492;
            4'h8: x_seed = 32'sh0000_2000; 4'h9: x_seed = 32'sh0000_1C71;
            4'hA: x_seed = 32'sh0000_1999; 4'hB: x_seed = 32'sh0000_1745;
            4'hC: x_seed = 32'sh0000_1555; 4'hD: x_seed = 32'sh0000_1249;
            4'hE: x_seed = 32'sh0000_1111; 4'hF: x_seed = 32'sh0000_1000;
            default: x_seed = 32'sh0001_0000;
        endcase
    end
    assign x_bus0 = x_seed;

    // NR pipeline
    nr_iteration_stage #(32,16) stage0(clk,rst,a_bus0,b_bus0,x_bus0,a_bus1,b_bus1,x_bus1);
    nr_iteration_stage #(32,16) stage1(clk,rst,a_bus1,b_bus1,x_bus1,a_bus2,b_bus2,x_bus2);
    nr_iteration_stage #(32,16) stage2(clk,rst,a_bus2,b_bus2,x_bus2,a_bus3,b_bus3,x_bus3);
    nr_iteration_stage #(32,16) stage3(clk,rst,a_bus3,b_bus3,x_bus3,a_bus4,b_bus4,x_bus4);

    // Final multiply
    reg signed [63:0] f_mul;
    always @(posedge clk) begin
        if (rst) f_mul <= 0;
        else f_mul <= a_bus4 * x_bus4;
    end
    assign quotient = f_mul >>> 16;
endmodule