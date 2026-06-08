# GEMM HLS Reference

Project target:

- Algorithm: 100x100 short PolyBench 2mm.
- Fixed Vivado part: `xc7k325tffv900-2`.
- Toolchain: Vivado/Vitis HLS 2023.2.

Borrowable ideas:

- tiling
- row-stationary dataflow
- systolic PE array concepts
- tile buffers
- read/compute/write dataflow
- accumulator chunking

Do not directly borrow:

- fixed 32x32 FP32 array defaults
- Alveo U50 assumptions
- URAM assumptions
- 512-bit AXI assumptions
- `v++` xclbin flow
- fixed 300 MHz assumptions

Adaptation guidance:

- Keep matrices generated internally from `seed`.
- Preserve the `sum`-only external result.
- Recompute capacity and timing against `xc7k325tffv900-2`.
- Validate final performance with post-route clock, not HLS estimate alone.
