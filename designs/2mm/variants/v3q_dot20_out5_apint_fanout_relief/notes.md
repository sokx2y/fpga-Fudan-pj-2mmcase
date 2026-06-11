# v3q_dot20_out5_apint_fanout_relief

## Intent

Hardware-specific timing-relief follow-up to v3p.

v3p is the current champion:

- DOT20 / OUT5
- initless A/B/C
- row ping-pong DATAFLOW
- no full tmp
- no full D
- DSP multiply latency 2
- local wide dot accumulator
- HLS `solution475`: 3534 cycles
- Vivado 4.6 ns: WNS +0.006 ns, DSP 840 / 840

The routed v3p critical paths are mostly route dominated and include:

- `i_plus_seed -> a_lane -> DSP48E1 input`
- short/carry paths around local lane generation

v3q keeps the same architecture and only tries to reduce timing/routing
pressure.

## Local Change From v3p

Source:

```text
designs/2mm/src/kernel_2mm_stage3q_dot20_out5_apint_fanout_relief.cpp
```

Changes:

- include `<ap_int.h>`;
- use `ap_int<24>` as the local dot-product accumulator;
- keep product truncation to `DATA_TYPE` before accumulation;
- keep final tmp/D cast back to `DATA_TYPE`;
- copy `a_lane` and `tmp_lane` into per-OUT-lane local arrays before DSP use.

Unchanged:

- DOT20 / OUT5;
- row ping-pong DATAFLOW;
- no full tmp / no full D;
- `lane_sum` remains 32-bit `int`;
- DSP multiply remains `bind_op op=mul impl=dsp latency=2`.

## Semantic Rationale

Original visible semantics are still:

```text
tmp = short(sum of short products)
D   = short(sum of short products)
sum = int reduction of short D
```

The dot accumulator is widened only locally.  Since every product is already
truncated to `short`, summing 100 products fits comfortably in signed 24-bit
for this benchmark's generated values.  The visible tmp/D values are still
cast back to `DATA_TYPE`.

## Success Criteria

- C-sim PASS for `seed=3`, `expected_sum=957248`.
- HLS latency stays close to v3p:

```text
target: <= 3600 cycles
acceptable: <= 3736 cycles
```

- HLS compute/consume row loops stay around v3p:

```text
compute_tmp_row ~= 32 cycles
consume_tmp_row ~= 33 cycles
```

- Vivado 4.6 ns WNS improves over v3p's +0.006 ns, or v3q passes a tighter
  clock such as 4.55 ns / 4.50 ns.

## Reject Criteria

- HLS latency rises above v3i/v3j territory without a timing benefit.
- DSP no longer maps to the full 840-DSP design shape.
- Vivado timing is worse than v3p at the same 4.6 ns constraint.

## Suggested HLS Command Context

```text
HLS_VARIANT=v3q_dot20_out5_apint_fanout_relief
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage3q_dot20_out5_apint_fanout_relief.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## What To Check First

After HLS:

- top latency cycles;
- `compute_tmp_row` latency;
- `consume_tmp_row` latency;
- DSP estimate;
- whether any recurrence or memory-port warning appears.

If HLS remains near 3534 cycles, export/run Vivado at:

```text
4.60 ns first
4.55 ns if 4.60 ns has positive margin
4.50 ns only if 4.55 ns is clean
```

## Result: solution475

HLS `solution475`:

- Target clock: 4.75 ns
- Estimated clock: 3.442 ns
- Latency: 3434 cycles
- Top row loop: 99 iterations, 34 cycles/iteration
- `compute_tmp_row`: 32 cycles
- `consume_tmp_row`: 32 cycles
- DSP estimate: 1000 / 840
- BRAM_18K: 0
- FF: 36596
- LUT: 47144

Interpretation:

- v3q improves v3p's HLS latency from 3534 to 3434 cycles.
- The one-cycle improvement in `consume_tmp_row` removes about 100 total
  cycles from the row ping-pong design.
- The main architecture remains unchanged from v3p, so this is a low-risk
  hardware-specific refinement rather than a new algorithmic schedule.

## Vivado 4.6 ns Result

Vivado implementation passes at 4.6 ns:

- Clock constraint: 4.600 ns
- WNS: +0.006 ns
- TNS: 0.000 ns
- WHS: +0.038 ns
- THS: 0.000 ns
- Route status: 99070 / 99070 routable nets fully routed
- Routing errors: 0
- Placed DSP: 840 / 840
- Placed LUT: 40515 / 203800
- Placed FF: 28055 / 407600
- Placed BRAM tile: 0 / 445

Conservative final runtime:

```text
runtime_ns = 3434 * 4.600 = 15796.4 ns
speedup_vs_CPU_4.632ms = 293.23x
```

Archived reports:

```text
designs/2mm/hls_proj/v3q_dot20_out5_apint_fanout_relief/vivado_impl_4p6_pass_20260612/
```

Archived implementation checkpoints:

```text
designs/2mm/hls_proj/v3q_dot20_out5_apint_fanout_relief/vivado_impl_4p6_pass_20260612/checkpoints/

kernel_2mm_opt.dcp
kernel_2mm_placed.dcp
kernel_2mm_physopt.dcp
kernel_2mm_routed.dcp
```

DRC/methodology notes:

- DRC critical warnings are `NSTD-1` and `UCIO-1`, caused by unspecified board
  I/O standard and pin locations on the bare HLS kernel ports.
- Methodology warnings are `TIMING-18`, missing input/output delay on bare
  top-level ports.
- These match the earlier kernel-only experiments and do not indicate a timing
  or routing failure.

Current interpretation:

v3q is the current performance champion.  It improves cycles versus v3p while
keeping the same 4.6 ns post-route pass.  However, WNS is still only +0.006 ns
and the worst paths remain route-dominated, so this architecture is very close
to its current physical timing boundary on `xc7k325tffv900-2`.
