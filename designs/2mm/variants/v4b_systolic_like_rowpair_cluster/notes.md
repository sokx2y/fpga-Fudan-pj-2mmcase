# v4b_systolic_like_rowpair_cluster

## Intent

V4B is the first explicit systolic-like / tiled PE-array attempt.  It is not a
drop-in GEMM-HLS port and not a 32x32 FP32/U50 design.  It is scaled to this
project's 100x100 `short` data and fixed `xc7k325tffv900-2` target.

Current champion to beat:

```text
v3q_dot20_out5_apint_fanout_relief
latency: 3434 cycles
post-route clock: 4.600 ns
runtime: 15796.4 ns
DSP: 840 / 840
WNS: +0.006 ns
```

## Source

```text
designs/2mm/src/kernel_2mm_stage4b_systolic_like_rowpair_cluster.cpp
```

## Structure

V4B uses:

```text
ROW_TILE_FACTOR = 2
DOT_UNROLL_FACTOR = 10
OUT_UNROLL_FACTOR = 5
K_CHUNK_COUNT = 10
```

Instead of computing one row with DOT20/OUT5, it computes two rows together with
DOT10/OUT5.  Each output cluster has:

- one shared B or C generated operand;
- one row-local A/tmp operand for row 0;
- one row-local A/tmp operand for row 1;
- two local `ap_int<24>` accumulators.

This keeps the active multiply count per compute/consume phase near V3q:

```text
2 rows * 5 outputs * 10 k-lanes = 100 products
```

## Scheduler Revision

The first source draft let Vitis HLS inline the row-pair clusters under an
outer `j0` loop with `PIPELINE II=1`.  During C synthesis, HLS started to
complete-unroll the inner K loops with factor 100 while trying to satisfy that
outer pipeline.  That was not the intended DOT10 architecture and made
scheduling impractically large.

Current source is changed to:

- keep row-pair cluster functions out-of-line with `INLINE off`;
- remove the outer `j0` pipeline pragma;
- pipeline the cluster-local K chunk loop instead;
- unroll only the `DOT_UNROLL_FACTOR=10` inner lane loop.

This should prevent the K=100 complete-unroll scheduler trap.  It may cost
cycles, so V4B should now be judged first as a synthesizable architecture probe,
then as a performance candidate.

## Why This Follows Prior Experience

- V3d proved initless A/B/C generation matters.
- V3f/V3i proved row ping-pong DATAFLOW is the right granularity.
- V3m proved DOT20/OUT5 class parallelism has the right cycle potential but is
  physically tight.
- V3o proved multi-cycle accumulator recurrence binding is dangerous.
- V3p/V3q proved local wide/narrow accumulation can preserve short semantics and
  close timing.
- V3q proved the current champion is at the 4.6 ns physical boundary, so V4B
  must change structure, not only pragmas.

## Success Criteria

- C-sim PASS for `seed=3`, `expected_sum=957248`.
- HLS latency:

```text
stretch target: <= 3434 cycles
acceptable:    <= 4238 cycles if timing improves materially
watch:         <= 7000 cycles for architectural learning only
```

- Vivado maps to 840 DSP or less.
- Final metric beats V3q:

```text
latency_cycles * post_route_clock_period_ns < 15796.4 ns
```

## Reject Criteria

- C-sim fails.
- HLS complete-unrolls the K loop with factor 100 again.
- HLS latency exceeds 7000 cycles.
- Row-pair functions show large recurrence or II warnings.
- HLS/Vivado DSP/resource shape exceeds the fixed device.
- Vivado 4.6 ns WNS is worse than V3q and cycles are not lower.

## Suggested HLS Command Context

```text
HLS_VARIANT=v4b_systolic_like_rowpair_cluster
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage4b_systolic_like_rowpair_cluster.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## Local C++ Smoke Test

Host compile/run, not HLS:

```text
expected_sum = 957248
actual_sum   = 957248
PASS
```

After the chunked scheduler revision, host compile/run still passes:

```text
expected_sum = 957248
actual_sum   = 957248
PASS
```

## HLS Result And Decision

Vitis HLS 2023.2, `solution475`:

```text
Report: designs/2mm/hls_proj/v4b_systolic_like_rowpair_cluster/solution475/syn/report/csynth.rpt
Latency: 19534 cycles
compute_tmp_rowpair: 381 cycles
consume_tmp_rowpair: 381 cycles
top row-pair loop: 18767 cycles over 49 row pairs
HLS estimated resources: DSP 200, FF 20601, LUT 28023, BRAM 0
```

The scheduler-friendly revision fixed the earlier complete-unroll trap, but it
also serialized too much of the row-pair work.  Each row-pair phase now has 20
output tiles, and each tile calls five out-of-line clusters with 17-cycle local
latency.  That preserves a regular local cluster but leaves most of the device's
DSP budget idle at the top level.

Decision:

```text
Reject current V4B as a performance candidate.
Do not spend Vivado implementation time on it unless the goal is routing
diagnostics only.
```

Reason:

```text
V4B latency is 19534 cycles, 5.69x V4A/V3q cycles.
Even an unrealistic 3.0 ns routed clock would be 58602 ns, far slower than
V4A's 15624.7 ns at 4.55 ns.
```
