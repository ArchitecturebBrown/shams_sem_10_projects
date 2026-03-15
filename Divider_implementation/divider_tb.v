// ==========================================================
// 3. Testbench - Verilog-2001
// ==========================================================
module tb_nr_divider;
    reg clk, rst;
    reg signed [31:0] da, db;
    wire signed [31:0] qq;
    real ra, rb, rq;

    nr_divider_top dut(clk,rst,da,db,qq);

    // Clock generation
    initial begin
        clk = 0; 
        forever #5 clk = ~clk; 
    end

    // Q16 to real conversion
    always @(posedge clk) begin
        ra = $itor(da) / 65536.0;
        rb = $itor(db) / 65536.0;
        rq = $itor(qq) / 65536.0;
    end

    // VCD waveform dumping
    initial begin
        $dumpfile("simulation.vcd");
        $dumpvars(0, tb_nr_divider);
    end

    // Test sequence
    initial begin
        rst = 1; da = 0; db = 1; #20; 
        rst = 0;

        da = 10 << 16; db = 2 << 16; #100;
        $display("10 / 2 = %0.6f", rq);

        da = 8 << 16; db = 4 << 16; #100;
        $display("8 / 4 = %0.6f", rq);

        da = 21 << 16; db = 3 << 16; #100;
        $display("21 / 3 = %0.6f", rq);

        #200 $finish;
    end
endmodule