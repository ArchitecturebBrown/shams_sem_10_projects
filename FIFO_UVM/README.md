# UVM Testbench for Asynchronous FIFO

## Overview

This project implements a UVM-based verification environment in SystemVerilog to verify an asynchronous FIFO design. The FIFO operates across two clock domains (write clock and read clock), and the testbench ensures correct data transfer, proper synchronization between domains, and correct behavior of full and empty flags.

## Objective

* Verify asynchronous FIFO functionality using UVM
* Handle communication between two clock domains
* Write randomized data until FIFO becomes full
* Read data until FIFO becomes empty
* Validate correct behavior of full and empty signals
* Ensure no illegal read/write operations occur

## Design Under Test (DUT)

The asynchronous FIFO includes:

* Write clock (`wclk`) and read clock (`rclk`)
* Active-low resets (`wrst_n`, `rrst_n`)
* Write/read enables (`w_en`, `r_en`)
* 8-bit data input and output
* `full` flag for write protection
* `empty` flag for read protection

The design uses pointer synchronization across clock domains.

## Verification Architecture

The UVM environment includes:

* Sequence Item
* Sequencer
* Driver
* Monitor
* Agent
* Environment
* Test

A virtual interface is used to connect the UVM components to the DUT.

## Test Scenario

### Write Phase

* Random data is written into the FIFO
* One transaction per write clock cycle
* Continues until `full = 1`

### Read Phase

* Data is read from the FIFO
* One transaction per read clock cycle
* Continues until `empty = 1`

## Monitoring

The monitor samples and prints all signals in the QuestaSim transcript:

* w_en
* r_en
* data_in
* data_out
* full
* empty

## Simulation (QuestaSim)

```bash
vlib work
vlog -sv design.sv fifo_mem.v rd_ctrl.v wr_ctrl.v sync_2ff.v
vlog -sv my_pkg.sv interface.sv testbench.sv

vsim -voptargs=+acc work.top
add wave -r *
run -all
```

## Expected Results

* FIFO fills until full is asserted
* FIFO empties until empty is asserted
* No UVM errors or fatal messages
* Correct behavior across two clock domains

## Conclusion

This project verifies the correct operation of an asynchronous FIFO using a UVM-based testbench. The environment ensures proper synchronization between clock domains and validates correct FIFO behavior during both write and read phases.
