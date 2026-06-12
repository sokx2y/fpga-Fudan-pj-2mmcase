# v3m_dot20_out5_initless_row_pingpong_dsp_pipeline

## Intent

Same-DSP-budget shape exploration based on the v3i champion.

This variant keeps:

- initless A/B/C
- row ping-pong DATAFLOW
- no full tmp
- no full D
- lane_sum reduction
- complete row-buffer partitioning
- `bind_op` DSP multiply with latency 2

## Local Change From v3i

Only the compute shape changes:

```text
v3i: DOT25 / OUT4 = 25 k-lanes * 4 output lanes = 100 products per stage
v3m: DOT20 / OUT5 = 20 k-lanes * 5 output lanes = 100 products per stage
```

The goal is not to use more DSPs than v3i.  The goal is to keep multiplier
pressure near the same level while reducing output tiles:

```text
OUT4: 100 columns / 4 = 25 output tiles
OUT5: 100 columns / 5 = 20 output tiles
```

If HLS can schedule the DOT20 reductions efficiently, row latency may drop
without requiring a tighter post-route clock than v3i.

## Why This Is Worth Trying

v3i at 4.6 ns:

```text
latency = 4238 cycles
post-route clock = 4.600 ns
runtime = 19494.8 ns
DSP = 800 / 840
```

If v3m reaches around 3900 cycles, it only needs about 5.0 ns post-route period
to beat v3i:

```text
19494.8 / 3900 = 4.999 ns
```

So v3m can win by reducing cycles even if the clock is a little slower than
v3i's 4.6 ns pass.

## Success Criteria

- C-sim PASS, seed=3, expected_sum=957248
- HLS latency materially below 4238 cycles
- DSP remains close to 800
- BRAM remains near zero
- Vivado routes at a period that gives lower runtime than v3i:
  `latency_cycles * post_route_period_ns < 19494.8 ns`

## Reject Criteria

- HLS latency is not better than v3i
- DSP drops far below 800, indicating lost parallelism
- HLS estimated clock or Vivado route becomes much worse
- OUT5 lane routing causes placement/routing instability

## Suggested HLS Command Context

```text
HLS_VARIANT=v3m_dot20_out5_initless_row_pingpong_dsp_pipeline
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage3m_dot20_out5_initless_row_pingpong_dsp_pipeline.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## How To Judge

Compare first against v3i:

```text
v3i HLS: latency 4238, estimated 3.442 ns, compute 36, consume 40, DSP 800
v3i Vivado: 4.600 ns pass, runtime 19494.8 ns
```

Initial decision rule:

```text
v3m <= 3900 cycles: run Vivado immediately
3900 < v3m < 4238 cycles: run Vivado if DSP is near 800 and estimated clock is reasonable
v3m >= 4238 cycles: reject unless Vivado timing looks dramatically easier
```

## Local Correctness Smoke Test

Host C++ compile/run with the project testbench:

```text
expected_sum = 957248
actual_sum   = 957248
PASS
```

## Result: solution475

HLS `solution475`:

- Target clock: 4.75 ns
- Estimated clock: 3.442 ns
- Latency: 3733 cycles
- Top row loop: 99 iterations, 37 cycles/iteration
- `compute_tmp_row`: 31 cycles
- `consume_tmp_row`: 35 cycles
- DSP: 1000 / 840
- BRAM_18K: 0 / 890
- FF: 50952 / 407600
- LUT: 62929 / 203800

Interpretation:

The DOT20 / OUT5 shape validates the performance idea: cycles drop from v3i's
4238 to 3733.  However, it is not implementable on `xc7k325tffv900-2` because
HLS estimates 1000 DSPs:

```text
compute_tmp_row: 500 DSP
consume_tmp_row: 500 DSP
total:           1000 DSP
available:        840 DSP
```

Do not run Vivado for this exact variant.  It exceeds the fixed device's DSP
capacity before placement and routing.

Follow-up direction:

Keep the successful OUT5 shape, but reduce the DOT lanes to fit the device.
The most promising follow-up is `DOT16 / OUT5`, which should target roughly
800 DSP total while preserving the lower 20-output-tile structure that made
v3m faster.

## Vivado 4.6 ns Result

Vivado synthesis/placement mapped the design into the fixed device even though
HLS estimated 1000 DSPs:

- Synth DSP: 840 / 840
- Placed DSP: 840 / 840
- Placed LUT: 55491 / 203800
- Placed FF: 29216 / 407600
- Placed BRAM tile: 0 / 445

Routed result at 4.6 ns:

- Route status: 106221 / 106221 routable nets fully routed
- Routing errors: 0
- WNS: -0.125 ns
- TNS: -35.077 ns
- Setup failing endpoints: 1090
- WHS: +0.056 ns

Archived reports:

```text
designs/2mm/hls_proj/v3m_dot20_out5_initless_row_pingpong_dsp_pipeline/vivado_impl_4p6_fail_20260611/
```

The design is physically routable but narrowly misses 4.6 ns timing.

Worst setup path:

```text
Source:
  grp_compute_tmp_row_fu_883/j_base_reg_17840_pp0_iter2_reg_reg[5]/C

Destination:
  grp_compute_tmp_row_fu_883/mul_16s_16s_32_3_1_U412/buff0_reg/A[22]

Data Path Delay:
  4.629 ns = logic 0.735 ns + route 3.894 ns

Logic Levels:
  4 (CARRY4=3 LUT4=1)
```

Interpretation:

The 4.6 ns miss is not a full routing failure.  The main setup issue is the
compute-side B operand generation path from `j_base` into DSP input registers.
The route share is about 84%, and the logic is the small integer expression
used for:

```cpp
B[k][j] = k + j - seed
```

Targeted follow-up:

Keep DOT20 / OUT5 and add timing relief for the generated B/C operands:

- precompute per-output-lane short `j - seed` terms;
- form B/C operands from local short terms instead of repeatedly using the
  wider `j_base` expression directly at each DSP input;
- preserve short truncation semantics;
- keep row ping-pong DATAFLOW and DSP pipeline binding.
