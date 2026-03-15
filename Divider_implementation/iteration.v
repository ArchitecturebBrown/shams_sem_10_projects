// ==========================================================
// 1. NR Stage (2-Cycle Balanced Pipeline) - Verilog-2001
// ==========================================================
module nr_iteration_stage #(parameter WIDTH = 32, FRAC = 16)(
    input clk, rst,
    input signed [WIDTH-1:0] a_in, b_in, x_in,
    output signed [WIDTH-1:0] a_out, b_out, x_out
);
    localparam signed [WIDTH-1:0] CONST_2 = 2 << FRAC;

    reg signed [WIDTH-1:0] a_r1, b_r1, x_r1;
    reg signed [WIDTH-1:0] a_r2, b_r2, x_r2;
    reg signed [63:0] mul1;

    // Stage 1
    always @(posedge clk) begin
        if (rst) begin
            mul1 <= 0; a_r1 <= 0; b_r1 <= 0; x_r1 <= 0;
        end else begin
            mul1 <= b_in * x_in;
            a_r1 <= a_in; b_r1 <= b_in; x_r1 <= x_in;
        end
    end

    // Stage 2
    always @(posedge clk) begin
        if (rst) begin
            x_r2 <= 0; a_r2 <= 0; b_r2 <= 0;
        end else begin
            x_r2 <= (x_r1 * (CONST_2 - (mul1 >>> FRAC))) >>> FRAC;
            a_r2 <= a_r1; b_r2 <= b_r1;
        end
    end

    assign a_out = a_r2;
    assign b_out = b_r2;
    assign x_out = x_r2;
endmodule

