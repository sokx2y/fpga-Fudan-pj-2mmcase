# V5B HLS 4.75 ns Result

Source project:

```text
designs/2mm/hls_proj/v5b_closed_form_row20
```

Archived report:

```text
kernel_2mm_csynth.rpt
```

Key result:

```text
Tool: Vitis HLS 2023.2
Part: xc7k325t-ffv900-2
Solution: solution475
Clock target: 4.75 ns
Estimated clock: 3.393 ns
Latency: 238 cycles
Interval: 239 cycles
Top loop: 5 row groups, 235 cycles
Resources: DSP 660, FF 85989, LUT 103494, BRAM 0
```

This confirms that the 20 row-lane design replicated the row engines rather
than sharing the single low-resource V5A engine.
