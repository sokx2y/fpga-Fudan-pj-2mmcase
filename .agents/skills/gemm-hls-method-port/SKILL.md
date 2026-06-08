---
name: gemm-hls-method-port
description: Use this skill when porting concrete GEMM-HLS architecture structures into 100x100 short 2mm architecture candidates without copying implementation code.
---

# GEMM-HLS Method Port

## When to use

Use this skill when a 2mm candidate borrows row-stationary scheduling, PE-array
structure, tile buffers, wavefront feeding, or chunked accumulation from
GEMM-HLS-style designs.

## Inputs to read

- `AGENTS.md`
- `designs/2mm/spec.json`
- `refs/method_library/gemm_hls_method_map.md`
- `refs/method_library/2mm_architecture_catalog.md`
- `.agents/skills/gemm-hls-reference/REFERENCE.md`

## Required workflow

1. Select the GEMM-HLS structure being ported.
2. Map it to 2mm internally generated A/B/C/tmp data.
3. Choose a scaled PE or tile shape, if applicable.
4. Define generate/compute/consume/reduce stages.
5. Define tmp tile buffer communication.
6. List correctness risks before implementation.
7. Reject platform assumptions that do not fit XC7K325T.

## Hard constraints

- The fixed Vivado part is `xc7k325tffv900-2`.
- Do not copy GEMM-HLS source code into this 2mm implementation.
- Do not assume 32x32 FP32, Alveo U50, URAM, 512-bit AXI, `v++`, xclbin, or
  300 MHz.
- Do not assume M/N/K are multiples of 32.

## Expected output

- GEMM-HLS-to-2mm mapping table.
- Stage decomposition.
- PE/tile scaling candidate.
- Correctness risk list.
- HLS implementation notes.

## What not to do

- Do not implement kernel code from this skill.
- Do not replace original 2mm semantics with external-matrix GEMM semantics.
- Do not use this as a baseline shortcut.

