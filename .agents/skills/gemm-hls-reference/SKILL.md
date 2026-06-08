---
name: gemm-hls-reference
description: Use this skill as a GEMM-HLS-to-2mm porting guide for mapping concrete GEMM structures into safe 2mm architecture candidates.
---

# GEMM HLS Reference

## When to use

Use this skill when an architecture candidate borrows concrete GEMM-HLS ideas:
row-stationary scheduling, PE arrays, tile buffers, process-one-tile structure,
wavefront feeding, or chunked accumulators.

## Inputs to read

- `AGENTS.md`
- `designs/2mm/spec.json`
- `refs/repo_notes/GEMM-HLS-borrowed.md`
- `refs/method_library/gemm_hls_method_map.md`
- `refs/method_library/2mm_architecture_catalog.md`
- `.agents/skills/gemm-hls-method-port/REFERENCE.md`

## Required workflow

1. Identify the GEMM-HLS structure being borrowed.
2. Use the mapping table in `REFERENCE.md` to translate it to 2mm.
3. Remove FP32, U50, URAM, 512-bit AXI, `v++`, xclbin, and 300 MHz
   assumptions.
4. Recompute PE shape, tile shape, buffering, and edge handling for 100x100
   `short`.
5. Preserve internally generated A/B/C/tmp and the `sum` output.
6. List HLS implementation notes and correctness risks before implementation.

## Hard constraints

- The part is fixed to `xc7k325tffv900-2`.
- Do not copy a GEMM-HLS implementation directly.
- Do not assume matrix dimensions are multiples of 32.
- Do not change the top interface `kernel_2mm(short seed, int *sum)`.

## Expected output

- GEMM-HLS-to-2mm mapping table.
- Adapted stage decomposition.
- PE/tile scaling recommendation.
- Correctness risk and reject conditions.
- HLS implementation notes.

## What not to do

- Do not treat GEMM-HLS as drop-in source code.
- Do not change the 2mm external interface.
- Do not replace the fixed Vivado part.
- Do not use scaled systolic design as baseline.

