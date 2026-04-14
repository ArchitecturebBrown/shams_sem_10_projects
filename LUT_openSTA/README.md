# Look-Up Table (LUT) Implementation

## Overview

This project presents the design and implementation of a Look-Up Table (LUT) as part of an Advanced Microelectronics Laboratory assignment. The objective is to achieve correct functional behavior, satisfy timing constraints, and analyze power consumption using standard ASIC design tools.

---

## Tools and Technologies Used

- **Hardware Description Language:** Verilog  
- **Simulation:** Icarus Verilog  
- **Waveform Viewer:** GTKWave  
- **Static Timing Analysis:** OpenSTA  
- **Technology Library / PDK:** Sky130A  

---

## Design Description

The implemented LUT is a mixed combinational-sequential design. It performs arithmetic and logical operations such as:

- AND  
- OR  
- Addition  
- Subtraction  

The combinational logic output is captured using a single flip-flop stage, ensuring synchronous output on the clock edge.

---

## Simulation

Functional verification was performed using:

- Icarus Verilog for compilation and simulation  
- GTKWave for waveform visualization  

The simulation results confirm correct functionality across all input combinations, with proper synchronization at the output register stage.

---

## Timing Analysis

Static timing analysis was performed using OpenSTA. The design meets all timing constraints:

- Setup Slack: ~ +0.10 ns  
- Hold Slack: ~ +2.07 ns  
- Worst Negative Slack (WNS): 0.00 ns  
- Total Negative Slack (TNS): 0.00 ns  

These results indicate that the design is fully timing-closed with no violations under the target clock period.

---

## Power Analysis

Power estimation results:

- Leakage Power: ~99.5%  
- Internal Power: ~0.3%  
- Switching Power: ~0.2%  

Leakage power dominates total power consumption, which is expected for small-scale digital designs in this technology node.

---

## Design Power Characteristics

- Combinational Logic Contribution: ~73.5%  
- Sequential Logic Contribution: ~26.5%  

This reflects the LUT architecture, where combinational logic dominates and a single flip-flop provides output registration.

---

## Conclusion

The LUT design implemented using the Sky130A PDK demonstrates correct functionality, timing compliance, and expected power behavior. The results confirm that the design is robust and successfully meets all design constraints.

---

## Authors

- Shams Tarek  
- Aya El Bakri  
- Hanna Mounir  
