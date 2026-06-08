# GEMM-HLS Method Map For 2mm

This note ports architecture methods from GEMM-HLS into this repository without
copying its implementation or platform assumptions.

## Sources Consulted

- `GEMM-HLS/README.md`
- `GEMM-HLS/src/types.h`
- `GEMM-HLS/src/systolic_row.h`
- `GEMM-HLS/src/gemm_general.h`
- `GEMM-HLS/src/gemm_general.cpp`
- `GEMM-HLS/tb/gemm_general_tb.cpp`

The local clone failed because GitHub was unreachable from this environment, so
the method pass used GitHub raw source views where available.

## Original GEMM-HLS Methods

GEMM-HLS is useful here as a hardware architecture reference:

- Systolic / PE-array organization.
- Row-stationary scheduling.
- A-row and B-tile buffering.
- Task decomposition into read, compute, and write stages.
- Tile-local processing such as `process_one_C_tile`.
- Diagonal or wavefront-style feeding for PE arrays.
- Chunked accumulation to manage parallel partial sums.

## Hardware Meaning

- PE array: maps repeated dot-product operations onto parallel multiply-accumulate
  lanes.
- Row-stationary scheduling: keeps one operand row or tile close to compute to
  reduce reloads.
- Tile buffers: decouple external or generated data production from compute.
- Read/compute/write pipeline: overlaps producer, compute, and consumer stages.
- Chunked accumulators: limit fan-in and reduce one giant critical-path
  reduction.

## Why It Cannot Be Copied Directly

Direct copying is forbidden and technically unsafe because this project differs:

- Data type is `short`, not default FP32.
- Matrix dimensions are fixed at 100, not necessarily multiples of 32.
- Inputs are generated internally from `seed`.
- External output is only `sum`, not a full output matrix.
- Target part is `xc7k325tffv900-2`, not Alveo U50.
- No URAM, HBM, 512-bit AXI, `v++`, xclbin, or fixed 300 MHz assumption.

## Migration To 100x100 Short 2mm

| GEMM-HLS structure | 2mm migration |
| --- | --- |
| 32x32 systolic array | Parameterized PE candidates: 10x10, 20x10, 20x20, 25x10, 25x20. |
| Row-stationary scheduling | A-row/tile stationary for GEMM1; tmp-row/tile stationary for GEMM2. |
| A cache / B buffer | BRAM/register tile buffers for generated A, B, C, and tmp. |
| `Task_Read_A_Row` | `Task_Generate_A_RowBlock` or `Task_Load_A_Tile`. |
| `Task_Read_B_All` | `Task_Generate_B_Tile` or `Task_Generate_C_Tile`. |
| `Task_Compute_All` | `Compute_AB_Tile` and `Consume_Tmp_Tile`. |
| `Task_Write_C_All` | `Reduce_Sum` or optional D store for equivalence testing. |
| `process_one_C_tile` | `process_one_tmp_tile` or `process_one_sum_tile`. |
| Diagonal feeding | Wavefront PE schedule candidate. |
| Chunked accumulators | Partial sum chunks for tmp and final sum reduction. |
| DATAFLOW read/compute/write | generate -> compute_tmp -> consume_tmp -> reduce_sum pipeline. |

## 2mm Architecture Candidates

- `tmp_tile_buffer_fused_gemm`: migrate read/compute/write staging into a
  communication path between GEMM1 and GEMM2.
- `row_stationary_tile_pipeline`: keep A or tmp tiles stationary to reduce
  reloads.
- `scaled_systolic_pe_array`: late-stage PE-array candidate after simpler
  variants are understood.
- `unrolled_dot_product_engine`: smaller Exploiter-style migration of PE
  multiply-accumulate lanes.
- `sum_only_no_D_store`: removes output matrix storage but requires equivalence
  proof for `short` intermediate semantics.

## HLS Implementation Notes

- Use fixed-size loops and explicit tile bounds for 100x100.
- Handle edge tiles because 100 is not divisible by every candidate tile or PE
  size.
- Keep producer/consumer DATAFLOW regions single-writer/single-reader.
- Keep FIFO depth explicit when tmp communication is streamed.
- Avoid a single huge reduction tree for final `sum`.
- Validate every algebraic transform against original `short` truncation
  behavior.

## Correctness Risks

- C-colsum and sum-only transforms may change when `short` intermediates are
  truncated or overflow.
- Fusing GEMM1 and GEMM2 may change the order or timing of tmp materialization.
- PE arrays can accidentally assume matrix dimensions divisible by PE shape.
- Chunked accumulation can change reduction order and integer overflow behavior.
- Removing D storage is safe only if the original observable behavior and
  intermediate type semantics are preserved.

