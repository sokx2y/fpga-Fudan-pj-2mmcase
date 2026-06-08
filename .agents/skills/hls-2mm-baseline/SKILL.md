---
name: hls-2mm-baseline
description: Use this skill when creating or reviewing a conservative correctness-first 2mm HLS baseline after the project skeleton is ready.
---

# HLS 2mm Baseline

## When to use

Use this skill only when the user explicitly asks to create or review the
baseline HLS source and C++ testbench.

## Inputs to read

- `AGENTS.md`
- `designs/2mm/spec.json`
- `refs/original_polybench/2mm.c`
- `refs/original_polybench/2mm.h`
- `refs/original_polybench/runtime.txt`

## Required workflow

1. Create `designs/2mm/src/kernel_2mm.h`.
2. Create `designs/2mm/src/kernel_2mm_baseline.cpp`.
3. Create `designs/2mm/tb/tb_kernel_2mm.cpp`.
4. Use deterministic test input, such as `seed = 3`.
5. Include a software golden reference in the C++ testbench.
6. Validate C simulation before considering synthesis results.

## Hard constraints

- The part is fixed to `xc7k325tffv900-2`.
- Keep the baseline conservative and correctness-first.
- Preserve the top interface `kernel_2mm(short seed, int *sum)`.
- Preserve the internally generated matrix semantics.

## Expected output

- Baseline source files.
- Testbench source file.
- C simulation result.
- C synthesis result when requested.

## What not to do

- Do not create this baseline during skeleton repair.
- Do not apply aggressive optimization.
- Do not create DSE variants from this skill alone.

