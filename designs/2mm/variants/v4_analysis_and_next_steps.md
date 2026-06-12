# V4 Analysis And Next Steps

Date: 2026-06-12

## Current Champion

V4A is the current post-route champion.

```text
Variant: v4a_output_stationary_pe_cluster
Source: designs/2mm/src/kernel_2mm_stage4a_output_stationary_pe_cluster.cpp
HLS latency: 3434 cycles
Vivado constraint: 4.550 ns
Routed WNS: +0.090 ns
Runtime: 15624.7 ns
Speedup vs 4.632 ms CPU baseline: 296.45x
Resources: LUT 40935 / 203800, FF 30207 / 407600, DSP 840 / 840, BRAM 0 / 445
Archive: designs/2mm/variants/v4a_output_stationary_pe_cluster/vivado_4p55_20260612/
```

V4A has the same cycle count as V3q but a better routed clock.  The top timing
paths remain route dominated, with the worst setup path showing 4.613 ns data
path delay split as 0.625 ns logic and 3.988 ns route.

## V4B Decision

Current V4B is rejected as a performance candidate.

```text
Variant: v4b_systolic_like_rowpair_cluster
Source: designs/2mm/src/kernel_2mm_stage4b_systolic_like_rowpair_cluster.cpp
HLS latency: 19534 cycles
HLS estimated resources: DSP 200, FF 20601, LUT 28023, BRAM 0
Decision: do not spend Vivado implementation time for performance ranking
```

Root cause:

```text
The scheduler-friendly fix avoided the K=100 complete-unroll trap by keeping
clusters out-of-line and moving the pipeline into the local K chunk loop.  That
made synthesis tractable, but serialized 20 output tiles and five cluster calls
per tile at the row-pair level.  The design therefore uses only 200 DSP at HLS
top level and gives away the 840-DSP-class parallelism that made V3q/V4A fast.
```

Even at an unrealistic 3.0 ns routed clock, V4B would be:

```text
19534 * 3.0 ns = 58602 ns
```

which is far slower than V4A:

```text
3434 * 4.55 ns = 15624.7 ns
```

## GEMM-HLS Method Borrowing

Reference repository:

```text
https://github.com/Shinei-Nouzen-Arch/GEMM-HLS
```

The local environment could not clone GitHub during this pass, so this note
uses the already recorded method map plus available repository-level source
information.  Direct implementation copying remains forbidden.

Useful ideas to keep:

```text
row-stationary scheduling
tile-local processing
PE-local state
generate/compute/consume stage decomposition
chunked accumulation
wavefront or systolic-like feeding as a late-stage experiment
```

Ideas to reject for this target:

```text
32x32 FP32 default
Alveo U50 assumptions
URAM cache assumptions
512-bit AXI assumptions
v++ / xclbin flow
fixed 300 MHz target
dimension-multiple-of-32 assumptions
```

For this 100x100 short 2mm design, the main lesson is not to port a full
systolic array first.  V4B shows that a regular PE-array shape can become too
serialized unless the top level keeps enough independent clusters active.

## Resource Headroom Interpretation

V4A uses all DSPs:

```text
DSP: 840 / 840
```

The remaining LUT/FF/BRAM headroom cannot directly create more multiplier
parallelism.  It is more useful for reducing clock period and routing pressure:

```text
1. local register replication near DSP clusters
2. cluster-local generated operand tables for B/C or j-dependent terms
3. smaller fanout domains for control and loop-carried signals
4. implementation directive sweeps and phys_opt exploration
5. optional floorplan/Pblock experiments if repeated routing shows stable hot regions
```

BRAM is not an obvious way to speed up V4A because the hot loops need many
parallel reads.  Moving `tmp_row` into BRAM would likely create a port bottleneck
unless it is used only as a tile buffer outside the DOT20/OUT5 inner engine.

## Recommended Next Candidates

### V4A Implementation Sweep

Keep the V4A RTL and test tighter clocks/directives first:

```text
4.50 ns
4.45 ns
4.40 ns only if 4.45 has nontrivial margin
```

This is low risk because cycles remain 3434 and the current 4.55 result has
+0.090 ns WNS.

### V4C Physical-Control Cluster

Start from V4A, not V4B.

Goal:

```text
Keep DOT20/OUT5 and 3434-cycle class, spend extra LUT/FF on local copies and
cluster-local operand/control generation to reduce route-dominated paths.
```

T1 checklist:

```text
Memory bandwidth: pass, no full tmp/D and same row ping-pong shape as V4A
DSP budget: must remain 840 routed, HLS estimate may exceed due mapping
Correctness risk: low if short truncation and int sum order stay unchanged
Timing risk: medium, because extra registers can either help fanout or add control
Reject if HLS latency exceeds 3534 cycles without a credible clock gain
```

### V4B2 Only As A Separate Research Route

If a systolic-like route is retried, it must keep 800-plus DSPs active at the
top level.  A useful V4B2 would need a static schedule that instantiates enough
row-pair clusters in parallel without triggering the K=100 complete-unroll
scheduler behavior.  This is a larger research branch, not the next champion
path.
