# v0_baseline Notes

`v0_baseline` is the Stage 0 HLS-migrated correctness baseline for the 2mm
project.

## Source Of Truth

- Root `src/2mm.c` and `src/2mm.h` are the original CPU / PolyBench-style
  baseline.
- `refs/original_polybench/` is an archival copy and must not be modified.
- `designs/2mm/src/kernel_2mm_baseline.cpp` is a clean HLS migration of the
  original root implementation behavior.

## Fixed Project Facts

- Toolchain: Vivado/Vitis HLS 2023.2.
- Target part: `xc7k325tffv900-2`.
- Dimensions: NI = NJ = NK = NL = 100.
- Data type: `short`.
- Top function: `kernel_2mm(short seed, int *sum)`.
- CPU baseline runtime: 4.632 ms.
- Later final metric: `latency_cycles * post_route_clock_period_ns`.

## Baseline Scope

This variant preserves the original two-stage computation:

1. Generate A, B, C, D, and tmp internally from `seed`.
2. Compute `tmp = A * B`.
3. Compute `D = tmp * C`.
4. Reduce D into `sum`.

No HLS optimization pragmas are used in Stage 0. This variant intentionally does
not use pipeline, unroll, DATAFLOW, array partition, C-colsum, sum-only,
systolic, or PE-array transformations.

## Manual Tool Policy

Vitis HLS and Vivado must be run manually by the user. The agent has not run
HLS, synthesis, cosimulation, export, or Vivado implementation for this variant.
