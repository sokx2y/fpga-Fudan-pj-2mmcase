# V4A Vivado 4.50 ns Archive

Source Vivado project:

```text
E:\Vivado\fpga\v4a_output_stationary_pe_cluster
```

Local archived files:

```text
kernel_2mm_routed.dcp
kernel_2mm_timing_summary_routed.rpt
kernel_2mm_utilization_placed.rpt
kernel_2mm_route_status.rpt
kernel_2mm_clock_utilization_routed.rpt
clockInfo.txt
runme.log
```

The raw `.dcp`, `.rpt`, and `.log` artifacts are ignored by git according to the
repository policy, so this README records the pushed summary.

Summary:

```text
Tool: Vivado 2023.2
Part: xc7k325tffv900-2
Clock constraint: 4.500 ns
Routed WNS: +0.052 ns
Routed WHS: +0.061 ns
Route status: fully routed, 0 routing errors
Resources: LUT 40981, FF 30049, DSP 840, BRAM 0
Latency: 3434 cycles
Runtime: 15453.0 ns
Speedup vs 4.632 ms CPU baseline: 299.75x
```
