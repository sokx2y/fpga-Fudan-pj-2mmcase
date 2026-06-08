# HLS 2mm Intake Reference

Project facts:

- Algorithm: PolyBench 2mm.
- Dimensions: NI = NJ = NK = NL = 100.
- Data type: `short`.
- Top function: `kernel_2mm(short seed, int *sum)`.
- Fixed Vivado part: `xc7k325tffv900-2`.
- CPU baseline: 4.632 ms.

Intake checklist:

- Read `refs/original_polybench/2mm.c`, `refs/original_polybench/2mm.h`, and
  `refs/original_polybench/runtime.txt` when those archival copies exist.
- Confirm that matrices are generated inside the kernel from `seed`.
- Confirm that final observable output is only `sum`.
- Treat `refs/old_hls_autotb/` as old generated RTL testbench reference only.

Do not reinterpret this project as an external-matrix GEMM accelerator.

