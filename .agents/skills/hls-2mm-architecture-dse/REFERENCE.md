# HLS 2mm Architecture DSE Reference

Project target:

- Fixed Vivado part: `xc7k325tffv900-2`.
- Toolchain: Vivado/Vitis HLS 2023.2.
- Final metric: `latency_cycles * post_route_clock_period_ns`.

## Architect Six-Element Format

Every architecture candidate must include:

- Hardware structure.
- Dataflow diagram.
- Storage hierarchy.
- GEMM1/GEMM2 communication path.
- Critical-path estimate.
- Resource scaling model and expected latency formula.

Local additions:

- Correctness risk.
- When to try.
- When to reject.

## Required Candidate Fields

Each architecture candidate must document:

- hardware structure
- dataflow diagram
- storage hierarchy
- GEMM1/GEMM2 communication path
- critical path estimate
- resource scaling model
- expected latency formula
- correctness risk
- when to try
- when to reject

## Candidate Catalog

Use `refs/method_library/2mm_architecture_catalog.md` as the authoritative
catalog for these candidates:

1. `baseline_two_gemm_bram_tmp`
2. `unrolled_dot_product_engine`
3. `sum_only_no_D_store`
4. `c_colsum_reduced_second_gemm`
5. `tmp_tile_buffer_fused_gemm`
6. `row_stationary_tile_pipeline`
7. `scaled_systolic_pe_array`

## Worker Classification

- Explorer: new structure with bounded scope.
- Exploiter: local tuning of a known-good structure.
- Innovator: novel schedule, communication, or PE-array structure.

## T1 Checklist Before HLS

- Pipeline placement.
- Unroll and array partition consistency.
- BRAM port pressure.
- DATAFLOW producer/consumer ownership.
- FIFO depth and deadlock risk.
- Reduction tree depth.
- Critical-path estimate.
- Post-synthesis structure consistency plan.

## Special Correctness Warnings

- `c_colsum_reduced_second_gemm` and `sum_only_no_D_store` may change `short`
  intermediate truncation or overflow semantics. Equivalence must be proven
  before treating performance as meaningful.
- `tmp_tile_buffer_fused_gemm` is the main two-GEMM communication design point.
- `scaled_systolic_pe_array` is late-stage only and must not be used as the
  baseline.

