# v3k_dot25_out4_initless_row_pingpong_a_fanout_relief

## Intent

Physical timing/fanout-relief experiment based on the v3i champion.

This variant keeps:

- DOT25 / OUT4
- initless A/B/C
- row ping-pong DATAFLOW
- no full tmp
- no full D
- lane_sum reduction
- complete row-buffer partitioning
- `bind_op` DSP multiply with latency 2

## Local Change From v3i

`compute_tmp_row` now materializes the current A row locally:

```cpp
DATA_TYPE a_row[NK];
#pragma HLS ARRAY_PARTITION variable=a_row complete dim=1

for (int k = 0; k < NK; ++k) {
  #pragma HLS UNROLL
  a_row[k] = (DATA_TYPE)(i + k + seed);
}
```

The DOT25/OUT4 compute loop then uses `a_row[k]` instead of recomputing
`i + k + seed` inside the pipelined multiply fabric.

## Why Try This

v3i's routed timing is route dominated. The archived 4.8 ns run shows critical
paths such as:

- `Data Path Delay: 4.331 ns`, route 82.315%
- `i` register fanout paths into the compute-side DSP fabric

This variant spends a small amount of local register/LUT fabric to reduce
direct fanout from `i/seed` into the compute array. The goal is not to reduce
HLS latency directly; the goal is to improve Vivado placement/routing at tight
clock constraints such as 4.60 ns and below.

## Success Criteria

- C-sim PASS, seed=3, expected_sum=957248
- HLS latency close to v3i's 4238 cycles
- DSP remains 800
- Vivado WNS improves versus v3i at the same tight clock
- Route congestion does not worsen

## Reject Criteria

- HLS latency rises materially above v3i
- DSP drops far below 800
- Vivado timing/route is worse than v3i

## Suggested HLS Command Context

```text
HLS_VARIANT=v3k_dot25_out4_initless_row_pingpong_a_fanout_relief
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage3k_dot25_out4_initless_row_pingpong_a_fanout_relief.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```
