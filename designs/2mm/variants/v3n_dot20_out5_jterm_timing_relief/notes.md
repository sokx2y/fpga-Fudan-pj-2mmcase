# v3n_dot20_out5_jterm_timing_relief

## Intent

Targeted timing-relief variant for v3m.

v3m is a strong cycles candidate:

```text
HLS latency: 3733 cycles
Vivado placed DSP: 840 / 840
Vivado route status: clean
Vivado 4.6 ns WNS: -0.125 ns
```

The routed miss is narrow, and the worst setup paths are not broad
unroutability.  They are compute-side generated-B paths from `j_base` into
DSP48E1 input registers.

## Local Change From v3m

Keep:

- DOT20 / OUT5
- initless A/B/C
- row ping-pong DATAFLOW
- no full tmp
- no full D
- lane_sum reduction
- complete row-buffer partitioning
- `bind_op` DSP multiply with latency 2

Change generated operand construction:

- precompute per-output-lane short `j - seed` terms for B;
- precompute per-output-lane short `seed - j` terms for C;
- form B/C operands from local short terms and short `k`;
- precompute short `i + seed` for A;
- preserve short truncation semantics.

## Why This Should Help

v3m worst path at 4.6 ns:

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

The goal is to avoid feeding the wide `j_base + k + lane - seed` expression
directly into every DSP input.  Local short terms may reduce CARRY depth and
fanout/route pressure into the DSP columns.

## Success Criteria

- C-sim PASS, seed=3, expected_sum=957248
- HLS latency remains close to v3m's 3733 cycles
- Vivado 4.6 ns WNS improves from v3m's -0.125 ns to non-negative
- DSP maps to no more than 840 after Vivado synthesis/placement

## Local Correctness Smoke Test

```text
expected_sum = 957248
actual_sum   = 957248
PASS
```

## Reject Criteria

- HLS latency rises enough to lose the v3m cycles advantage
- HLS/Vivado resource usage grows materially beyond v3m
- Worst path remains the same j-base-to-DSP path with no timing improvement

## Suggested HLS Command Context

```text
HLS_VARIANT=v3n_dot20_out5_jterm_timing_relief
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage3n_dot20_out5_jterm_timing_relief.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## Result: solution475

HLS `solution475`:

- Target clock: 4.75 ns
- Latency: 3434 cycles
- Top row loop: 99 iterations, 34 cycles/iteration
- `compute_tmp_row`: 32 cycles
- `consume_tmp_row`: 32 cycles
- DSP estimate: 1000 / 840
- BRAM_18K: 0
- FF: 36596
- LUT: 47144

This validates that the j-term localization can improve HLS scheduling and
reduce LUT/FF pressure versus v3m.

## Result: solution46 / Vivado 4.6 ns

HLS `solution46`:

- Target clock: 4.60 ns
- Estimated clock: 3.525 ns
- Latency: 3736 cycles
- Top row loop: 99 iterations, 37 cycles/iteration
- `compute_tmp_row`: 34 cycles
- `consume_tmp_row`: 34 cycles

Vivado 4.6 ns implementation:

- Route status: 99412 / 99412 routable nets fully routed
- Routing errors: 0
- Placed DSP: 840 / 840
- Placed LUT: 40534 / 203800
- Placed FF: 28395 / 407600
- Placed BRAM tile: 0 / 445
- WNS: -0.230 ns
- TNS: -229.771 ns
- Setup failing endpoints: 3392
- WHS: +0.065 ns

Archived reports:

```text
designs/2mm/hls_proj/v3n_dot20_out5_jterm_timing_relief/vivado_impl_4p6_fail_20260611/
```

The original v3m `j_base -> DSP input` path is no longer the only dominant
failure mode.  v3n's routed worst paths move into compute-side short
accumulation and DSP input fanout:

```text
add_ln29_* register -> add_ln29_* register
Data Path Delay: 4.380 ns = logic 0.876 ns + route 3.504 ns
Logic Levels: 5 (CARRY4=4 LUT3=1)

add_ln60_3_reg -> DSP48E1 A input
Data Path Delay: 4.343 ns = logic 0.223 ns + route 4.120 ns
Logic Levels: 0
```

Interpretation:

v3n improves HLS cycles/resource shape, but the 4.6 ns miss broadens into
accumulator carry chains and high-route DSP input nets.  The next code-side
attempt should keep DOT20 / OUT5 and target the `acc + product` short
accumulation path directly, even if it costs a small number of cycles.
