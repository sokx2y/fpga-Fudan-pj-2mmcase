# GEMM-HLS Borrowed Ideas

This project borrows high-level GEMM architecture ideas from GEMM-HLS, not
source code or platform assumptions.

## Borrowed

- Tiling.
- Row-stationary dataflow.
- Systolic / PE array concepts.
- Tile buffers.
- Read/compute/write pipeline structure.
- Accumulator chunking.

## Not Borrowed

- No direct copy of 32x32 FP32 designs.
- No Alveo U50 assumption.
- No URAM assumption.
- No 512-bit AXI assumption.
- No `v++` xclbin flow.
- No fixed 300 MHz assumption.

## Local Adaptation

Any borrowed GEMM idea must be resized and revalidated for 100x100 `short`
PolyBench 2mm, internally generated matrices, `sum` as the only external
result, Vivado/Vitis HLS 2023.2, and the fixed Vivado part
`xc7k325tffv900-2`.
