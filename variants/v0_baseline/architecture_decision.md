# Architecture Decision: v0_baseline

## Decision Summary

- Variant: `v0_baseline`
- Architecture: `baseline_two_gemm_bram_tmp`
- Worker class: Explorer baseline / correctness anchor
- T1 checklist status: pass for source creation; pending manual C simulation
- Decision: `pending_hls_run`

This decision records the Stage 0 correctness-first migration from the root
PolyBench-style CPU source to an HLS source layout. It is not an optimization.

## 1. Hardware Structure

The baseline keeps the original full-matrix structure:

- Local A matrix: `A[100][100]`
- Local B matrix: `B[100][100]`
- Local C matrix: `C[100][100]`
- Local D matrix: `D[100][100]`
- Local tmp matrix: `tmp[100][100]`

The top function remains:

```cpp
void kernel_2mm(short seed, int *sum)
```

## 2. Dataflow Diagram

```text
seed
  -> generate A/B/C/D/tmp
  -> GEMM1: tmp = A * B
  -> GEMM2: D = tmp * C
  -> reduce D into sum
```

## 3. Storage Hierarchy

All matrices are local fixed-size arrays in the HLS function. Stage 0 does not
apply array partitioning, explicit BRAM binding, streaming, or DATAFLOW
channels. The tool is allowed to infer storage during synthesis.

## 4. GEMM1/GEMM2 Communication Path

GEMM1 writes the complete `tmp[100][100]` matrix. GEMM2 reads the complete tmp
matrix only through this local array. There is no streaming, fusion, or
producer/consumer FIFO in Stage 0.

## 5. Critical-Path Estimate

The expected critical operation is the multiply-accumulate inside the inner
loops:

```text
short accumulator += short * short
```

The final D reduction may also contribute to timing. Because Stage 0 has no
pipeline, unroll, or dataflow pragmas, this is a correctness anchor rather than
a timing-optimized design.

## 6. Resource And Latency Model

Storage scales with five 100x100 `short` matrices. Compute latency is expected
to be dominated by:

- GEMM1: roughly `100 * 100 * 100` inner multiply-accumulate iterations.
- GEMM2: roughly `100 * 100 * 100` inner multiply-accumulate iterations.
- Final reduction: `100 * 100` additions.

The final runtime metric is not known yet. It must later be computed as:

```text
runtime_ns = latency_cycles * post_route_clock_period_ns
```

## T1 Checklist

- Pipeline: none by design.
- Unroll: none by design.
- Array partition: none by design.
- DATAFLOW: none by design.
- FIFO depth: not applicable.
- Critical path: pending HLS synthesis report.
- Post-synthesis consistency check: pending manual HLS run.
- Correctness: pending manual C simulation.

## Rejected Stage 0 Changes

- No C-colsum transform.
- No sum-only transform.
- No tmp/D type widening to int matrices.
- No systolic or PE-array structure.
- No root `src/` or `tb/` edits.
