# v3j_dot25_out4_initless_row_pingpong_dsp_mreg

## Intent

XC7K325T DSP48E1 pipeline experiment based on the v3i champion.

This variant keeps the successful architecture:

- DOT25 / OUT4
- initless A/B/C
- row ping-pong DATAFLOW
- no full tmp
- no full D
- lane_sum reduction
- complete row-buffer partitioning from v3i

## Local Change From v3i

The only intended architectural change is the DSP multiply wrapper:

```cpp
int product = ((int)lhs) * ((int)rhs);
#pragma HLS bind_op variable=product op=mul impl=dsp latency=4
return (DATA_TYPE)(((int)acc) + product);
```

Compared with v3i's `latency=2` short product wrapper, v3j asks HLS for a
deeper DSP multiply pipeline and keeps the product wide until the final short
accumulation cast. This is closer to the original compound-assignment semantics
and is meant to respond to Vivado's routed `DPOP-2` warnings about DSP MREG
output pipelining.

## Success Criteria

- C-sim PASS, seed=3, expected_sum=957248
- HLS latency close to v3i's 4238 cycles
- DSP remains close to 800
- Vivado at 4.8 ns remains clean, then test tighter clocks such as 4.65 ns or
  4.62 ns
- Routed DRC should reduce DPOP-2 warnings, or timing should improve enough to
  justify keeping the extra pipeline request

## Reject Criteria

- HLS latency rises materially above v3i
- DSP drops far below 800
- Vivado route/timing becomes worse than v3i
- DPOP-2 remains unchanged and WNS does not improve

## Source

`designs/2mm/src/kernel_2mm_stage3j_dot25_out4_initless_row_pingpong_dsp_mreg.cpp`

## Suggested HLS Command Context

Use the existing `run_hls.tcl` flow with:

```text
HLS_VARIANT=v3j_dot25_out4_initless_row_pingpong_dsp_mreg
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage3j_dot25_out4_initless_row_pingpong_dsp_mreg.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```
