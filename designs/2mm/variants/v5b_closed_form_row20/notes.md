# v5b_closed_form_row20

## Intent

Use the low resource footprint of V5A to buy latency.  V5B replicates the
closed-form row engine 20 times so that the 100 rows are processed in five row
groups instead of 100 serialized row iterations.

## Source

```text
designs/2mm/src/kernel_2mm_stage5b_closed_form_row20.cpp
```

It reuses the V5A multi-seed correctness testbench:

```text
designs/2mm/tb/tb_kernel_2mm_multi_seed.cpp
```

## Parameters

```text
MOMENT_UNROLL_FACTOR = 20
OUT_UNROLL_FACTOR    = 20
ROW_UNROLL_FACTOR    = 20
```

## Expected HLS Behavior

V5A report:

```text
Top latency: 4704 cycles
Per-row loop: 100 iterations, about 47 cycles each
Resources: 33 DSP, 5230 FF, 6114 LUT, 0 BRAM
```

V5B should replicate row engines and reduce the top row loop to five groups.
The top group body uses explicit template-instantiated row lanes inside a
`DATAFLOW` region so HLS has a clear signal to schedule the 20 rows
concurrently rather than sharing one row engine.

Expected first-pass resource class:

```text
DSP: roughly 600-700
BRAM: 0
Latency target: far below 3434 cycles
```

## Suggested HLS Command

From `designs/2mm`:

```text
$env:HLS_VARIANT="v5b_closed_form_row20"
$env:HLS_CLOCK_PERIOD_NS="4.75"
$env:HLS_SRC_FILES="../src/kernel_2mm_stage5b_closed_form_row20.cpp ../src/kernel_2mm.h"
$env:HLS_TB_FILES="../tb/tb_kernel_2mm_multi_seed.cpp"
& "E:\Xilinx\Vitis_HLS\2023.2\bin\vitis_hls.bat" -nolog -f scripts\run_hls.tcl
```

## Status

V5B is validated and is the current post-route runtime champion.

Local host compile/run with the Vitis HLS 2023.2 `ap_int.h` header passed:

```text
Command:
g++ -std=c++14 -I E:\Xilinx\Vitis_HLS\2023.2\include \
  designs\2mm\src\kernel_2mm_stage5b_closed_form_row20.cpp \
  designs\2mm\tb\tb_kernel_2mm_multi_seed.cpp \
  -o .codex_tmp\v5b_multi_seed.exe

Result:
seed = 0, expected_sum = -1555520, actual_sum = -1555520
seed = 1, expected_sum = 1293888, actual_sum = 1293888
seed = 2, expected_sum = 324288, actual_sum = 324288
seed = 3, expected_sum = 957248, actual_sum = 957248
seed = 4, expected_sum = -3116608, actual_sum = -3116608
seed = 7, expected_sum = 2575680, actual_sum = 2575680
seed = 31, expected_sum = -2434752, actual_sum = -2434752
seed = -3, expected_sum = -988096, actual_sum = -988096
seed = 123, expected_sum = 2135872, actual_sum = 2135872
seed = 32767, expected_sum = 3328320, actual_sum = 3328320
seed = 32760, expected_sum = 944064, actual_sum = 944064
seed = -32768, expected_sum = -1555520, actual_sum = -1555520
seed = -32700, expected_sum = -1027648, actual_sum = -1027648
seed = 30000, expected_sum = 650176, actual_sum = 650176
seed = -30000, expected_sum = 39872, actual_sum = 39872
PASS
```

HLS report check:

```text
1. Confirm the top loop is five row groups, not 100 serialized rows.
2. Confirm DSP rises into the expected resource-spending class.
3. Confirm HLS latency is materially below 3434 cycles before Vivado export.
```

## HLS Result

Vitis HLS 2023.2, `solution475`:

```text
Report: designs/2mm/hls_proj/v5b_closed_form_row20/solution475/syn/report/kernel_2mm_csynth.rpt
Archive: designs/2mm/variants/v5b_closed_form_row20/hls_4p75_20260612/
Part: xc7k325t-ffv900-2
Clock target: 4.75 ns
Estimated clock: 3.393 ns
Latency: 238 cycles
Interval: 239 cycles
Top row loop: 5 row groups, 235 cycles total, 47 cycles/group
HLS resources: DSP 660, FF 85989, LUT 103494, BRAM 0
```

This confirms the intended row-level replication:

```text
20 row lanes * 33 DSP/lane = 660 DSP
100 rows / 20 lanes = 5 row groups
```

Compared with V5A:

```text
V5A: 4704 cycles, 33 DSP
V5B:  238 cycles, 660 DSP
Cycle reduction: 4704 / 238 = 19.76x
```

Compared with the V4A mainline:

```text
V4A: 3434 cycles
V5B:  238 cycles
Cycle reduction: 3434 / 238 = 14.43x
```

## Vivado 4.50 ns Routed Result

Vivado 2023.2 implementation result from:

```text
E:\Vivado\fpga\v5b_closed_form_row20
```

Archived under:

```text
designs/2mm/variants/v5b_closed_form_row20/vivado_4p50_20260612/
```

Key result:

```text
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

This supersedes V4A by final metric:

```text
V4A: 3434 * 4.500 ns = 15453.0 ns
V5B:  238 * 4.500 ns =  1071.0 ns
Delta: -14382.0 ns
Runtime speedup vs V4A: 14.43x
```

The routed WNS of `+0.210 ns` suggests a tighter clock sweep may be possible,
but the accepted recorded result is already a large post-route win at 4.50 ns.
