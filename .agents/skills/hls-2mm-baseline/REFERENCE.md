# HLS 2mm Baseline Reference

Baseline target:

- Top function: `kernel_2mm`.
- Fixed Vivado part: `xc7k325tffv900-2`.
- Source header: `designs/2mm/src/kernel_2mm.h`.
- Source implementation: `designs/2mm/src/kernel_2mm_baseline.cpp`.
- Testbench: `designs/2mm/tb/tb_kernel_2mm.cpp`.

Rules:

- Keep the baseline conservative and correctness-first.
- Use deterministic test seed, for example `seed = 3`.
- Include a software golden reference in the C++ testbench.
- Require C simulation correctness.
- Require C synthesis to run before using HLS reports.
- Do not apply aggressive optimization in the baseline.

