# V5B Vivado 4.50 ns Routed Result

Source project:

```text
E:\Vivado\fpga\v5b_closed_form_row20
```

Archived reports:

```text
clockInfo.txt
kernel_2mm_route_status.rpt
kernel_2mm_timing_summary_routed.rpt
kernel_2mm_utilization_placed.rpt
```

Key result:

```text
Tool: Vivado 2023.2
Part: xc7k325tffv900-2
Constraint: 4.500 ns
Clock frequency: 222.222 MHz
Routed WNS: +0.210 ns
Routed WHS: +0.047 ns
Route status: 95766 / 95766 routable nets fully routed, 0 routing errors
Placed resources: LUT 48766 / 203800, FF 50960 / 407600, DSP 620 / 840, BRAM 0 / 445
Latency: 238 cycles
Final runtime: 238 * 4.500 ns = 1071.0 ns
Speedup vs 4.632 ms CPU baseline: 4324.93x
```

Comparison:

```text
V4A: 3434 * 4.500 ns = 15453.0 ns
V5B:  238 * 4.500 ns =  1071.0 ns
Runtime speedup vs V4A: 14.43x
```
