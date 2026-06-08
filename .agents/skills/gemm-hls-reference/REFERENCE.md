# GEMM-HLS To 2mm Porting Guide

Project target:

- Algorithm: 100x100 short PolyBench 2mm.
- Fixed Vivado part: `xc7k325tffv900-2`.
- Toolchain: Vivado/Vitis HLS 2023.2.
- External interface: `kernel_2mm(short seed, int *sum)`.

## Mapping Table

| GEMM-HLS idea | 2mm port |
| --- | --- |
| 32x32 systolic array | Parameterized PE array candidates: 10x10, 20x10, 20x20, 25x10, 25x20. |
| Row-stationary scheduling | A-row/tile stationary for GEMM1; tmp-row/tile stationary for GEMM2. |
| A cache | Generated A row/tile buffer in registers or BRAM. |
| B buffer | Generated B tile buffer for GEMM1; generated C tile buffer for GEMM2. |
| `Task_Read_A_Row` | `Task_Generate_A_RowBlock` or `Task_Load_A_Tile`. |
| `Task_Read_B_All` | `Task_Generate_B_Tile` or `Task_Generate_C_Tile`. |
| `Task_Compute_All` | `Compute_AB_Tile` and `Consume_Tmp_Tile`. |
| `Task_Write_C_All` | `Reduce_Sum` or optional D-store validation. |
| `process_one_C_tile` | `process_one_tmp_tile` or `process_one_sum_tile`. |
| diagonal feeding | wavefront PE schedule candidate. |
| chunked accumulators | partial sum chunks for tmp and final sum. |
| DATAFLOW read/compute/write | generate -> compute_tmp -> consume_tmp -> reduce_sum. |

## HLS Implementation Notes By Mapping

### PE Array Scaling

- Start with shapes that divide or nearly divide 100.
- Handle edge tiles explicitly.
- Estimate DSP count as PE count times active MAC lanes.
- Reject any shape that requires unreachable BRAM ports or collapses WNS.

### Row-Stationary Scheduling

- Keep generated A rows stationary for GEMM1 when sweeping B tiles.
- Keep tmp rows stationary for GEMM2 when sweeping C tiles.
- Avoid assuming external memory loads; A, B, and C are generated internally.

### Tile Buffers

- Use BRAM/register tile buffers for A, B, C, and tmp.
- Size buffers against XC7K325T capacity.
- Keep one writer and one reader for DATAFLOW channels.

### Tmp Communication

- `tmp_tile_buffer_fused_gemm` is the main communication candidate.
- Producer must finish all values needed by the consumer tile.
- FIFO or tile-buffer depth must be explicit.

### Stage Decomposition

Use:

`generate -> compute_tmp -> consume_tmp -> reduce_sum`

Each stage needs clear ownership. Avoid multi-consumer tmp streams unless a
buffering plan is documented.

### Wavefront Feeding

- Treat as a late-stage candidate only.
- Validate indexing and fill/drain cycles for 100x100 edge tiles.
- Expect routing and WNS risk on `xc7k325tffv900-2`.

### Chunked Accumulation

- Use partial sums to avoid a single huge reduction tree.
- Verify overflow and truncation behavior against the original `short`
  semantics.

## Forbidden Direct Ports

- No 32x32 FP32 default.
- No Alveo U50.
- No URAM.
- No 512-bit AXI.
- No `v++` xclbin flow.
- No fixed 300 MHz assumption.
- No M/N/K multiple-of-32 assumption.

## Correctness Risks

- C-colsum and sum-only transforms may change intermediate truncation semantics.
- Removing D storage is legal only after equivalence is proven.
- Chunked reductions can change overflow behavior.
- Fused tmp communication can drop or reuse tmp values incorrectly.
- PE-array edge tiles can silently miss rows or columns.
