# v4b_systolic_like_rowpair_cluster Architecture Decision

## Candidate

V4B is the high-risk systolic-like / tiled PE-array branch after the V3q
champion and V4A output-stationary cluster.  It deliberately avoids a full large
systolic array on the first attempt.  Instead, it uses a two-row PE cluster:

```text
ROW_TILE=2
DOT_UNROLL_FACTOR=10
OUT_UNROLL_FACTOR=5
```

The intent is to preserve the V3q work envelope while making B/C operand sharing
and PE placement more regular:

```text
2 rows * 5 output lanes * 10 k-lanes = 100 active products per phase
```

This mirrors the V3q DOT20 / OUT5 active work count without blindly doubling the
already full-DSP design.

## Scheduler Revision

The first V4B draft placed `#pragma HLS PIPELINE II=1` on the OUT5 `j0` loop
while the row-pair cluster functions were fully inlined.  Vitis HLS 2023.2 then
treated the inner K loop as a complete unroll while trying to satisfy the outer
pipeline, producing messages such as:

```text
Loop 'VITIS_LOOP_63_1' is marked as complete unroll implied by the pipeline pragma
Unrolling loop 'VITIS_LOOP_63_1' completely with a factor of 100
```

The current source avoids that scheduler trap:

- cluster functions are `INLINE off`;
- the OUT5 `j0` loop is not pipelined;
- each cluster has an explicit K chunk loop with `K_CHUNK_COUNT=10`;
- only the inner `DOT_UNROLL_FACTOR=10` lane loop is fully unrolled.

This is expected to synthesize more reliably, at the cost of a more conservative
cycle target.

## V3 Lessons Carried Forward

- Keep internally generated A/B/C; do not restore initialization/storage loops.
- Keep no full `D` storage and no final D reread.
- Keep row/tile ping-pong DATAFLOW; scalar streams were too fine-grained.
- Preserve `short` product truncation and final `short` tmp/D truncation.
- Use local `ap_int<24>` dot accumulators, following V3q.
- Do not latency-bind the accumulator recurrence; V3o showed that explodes row
  latency.
- Treat HLS estimates as screening only; final winner requires Vivado route.
- Respect the 840-DSP physical boundary on `xc7k325tffv900-2`.

## GEMM-HLS To 2mm Mapping

| GEMM-HLS idea | V4B 2mm mapping |
|---|---|
| Row-stationary PE array | Two adjacent 2mm rows are stationary inside a row-pair cluster. |
| B/C tile sharing | Generated B/C operands are shared across the two row lanes inside each cluster. |
| Process-one-C-tile | `compute_tmp_rowpair` and `consume_tmp_rowpair` process one OUT5 tile at a time. |
| Chunked accumulation | Each row/output cluster owns local `ap_int<24>` accumulation. |
| Tile communication | Ping-pong row-pair tmp buffers communicate from GEMM1 to GEMM2. |
| Wavefront/systolic style | Scaled down to a row-pair PE cluster, not a full 20x20 array. |

## Architect Six Elements

1. Hardware structure:
   A row-pair PE cluster computes two adjacent tmp rows together.  Five
   output-lane clusters are instantiated for each OUT5 tile; each cluster has
   two row accumulators and ten k-lanes.  The consumer mirrors this structure
   for two D rows.

2. Dataflow diagram:
   `compute_tmp_rowpair(pair N) -> ping/pong tmp pair -> consume_tmp_rowpair(pair N-1) -> row/lane sums -> final sum`

3. Storage hierarchy:
   Four fully partitioned 100-element row buffers:
   `tmp_ping0/tmp_ping1/tmp_pong0/tmp_pong1`.  `lane_sum[2][5]` is fully
   partitioned.  No full `tmp[100][100]`, no full `D`, no BRAM tile assumption.

4. GEMM1/GEMM2 communication:
   GEMM1 communicates two complete tmp rows at a time through ping-pong row-pair
   buffers.  GEMM2 consumes the previous row pair while GEMM1 computes the next
   row pair.

5. Critical-path estimate:
   Sharing B/C operands inside a row-pair cluster should make operand generation
   more local and regular than independent row engines.  The main timing risks
   are doubled tmp-row buffer width, more local row-pair control, and HLS
   flattening the cluster into a broad mux network.

6. Resource and latency model:
   The intended active product count per K chunk stays near V3q's safe physical
   envelope.  HLS may still estimate above 840 DSP, but Vivado must map to 840
   or less.  HLS latency is judged first for scheduler viability, then for
   performance:

```text
stretch target: <= 3434 cycles
acceptable:    <= 4238 cycles if timing improves materially
watch:         <= 7000 cycles for architectural learning only
reject:         scheduler complete-unrolls K=100 again or cycles explode beyond 7000
```

## Worker Classification

Innovator.

This changes the compute granularity from single-row DOT20/OUT5 to row-pair
DOT10/OUT5 PE clusters.  It borrows GEMM-HLS row-stationary/PE-array structure
but keeps the 2mm short semantics and fixed device boundary.

## T1 Hardware Checklist

| Check | Result | Note |
|---|---|---|
| Fixed part remains `xc7k325tffv900-2` | Pass | No alternate part. |
| Vitis/Vivado HLS 2023.2 compatibility | Pass | Plain C++ templates/pragmas, no 2025.2 APIs. |
| Alveo/U50/HBM/URAM/v++ assumptions | Pass | None introduced. |
| Top interface unchanged | Pass | `kernel_2mm(short seed, int *sum)`. |
| Short semantics preserved | Pass | Product truncates to `short`; tmp and D cast back to `short`. |
| No C-colsum algebraic rewrite | Pass | Avoids the risky transform. |
| DATAFLOW ownership | Pass | One producer and one consumer per ping-pong row-pair buffer. |
| Edge handling | Pass | `NI=100` is divisible by `ROW_TILE=2`; OUT/DOT loops cover 100 exactly. |
| DSP pressure | Watch | Intended active shape is 100 products per phase; must verify reports. |
| Timing/routing | Watch | Chunked cluster should avoid full K unroll but may cost cycles. |

## T1 Decision

Allowed to enter T2 HLS synthesis after local C-sim / host smoke testing.

Source:

```text
designs/2mm/src/kernel_2mm_stage4b_systolic_like_rowpair_cluster.cpp
```

Suggested HLS command context:

```text
HLS_VARIANT=v4b_systolic_like_rowpair_cluster
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage4b_systolic_like_rowpair_cluster.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

Proceed to Vivado only if HLS cycles are competitive and the function reports
show row-pair latency did not explode.
