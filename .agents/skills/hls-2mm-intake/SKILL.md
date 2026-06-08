---
name: hls-2mm-intake
description: Use this skill when verifying the original PolyBench 2mm facts, archived sources, interface, and baseline CPU runtime before HLS work.
---

# HLS 2mm Intake

## When to use

Use this skill before creating a baseline, modifying scripts, or planning design
space exploration.

## Inputs to read

- `src/2mm.c`
- `src/2mm.h`
- `runtime.txt`
- `refs/original_polybench/2mm.c`
- `refs/original_polybench/2mm.h`
- `refs/original_polybench/runtime.txt`
- `designs/2mm/spec.json`

## Required workflow

1. Confirm NI, NJ, NK, and NL are all 100.
2. Confirm `DATA_TYPE` is `short`.
3. Confirm top function signature is `kernel_2mm(short seed, int *sum)`.
4. Confirm matrices are generated internally from `seed`.
5. Confirm the only external result is `sum`.
6. Confirm CPU baseline is 4.632 ms.
7. Confirm the fixed part remains `xc7k325tffv900-2`.

## Hard constraints

- Treat `refs/original_polybench/` as read-only archive.
- Treat `refs/old_hls_autotb/` as old generated RTL testbench reference only.
- Do not reinterpret this case as an external matrix GEMM accelerator.

## Expected output

- Inventory summary.
- Confirmed project facts.
- Any mismatch between source files, archive files, and `spec.json`.

## What not to do

- Do not delete, move, or overwrite original files.
- Do not implement a baseline kernel.
- Do not start optimization.

