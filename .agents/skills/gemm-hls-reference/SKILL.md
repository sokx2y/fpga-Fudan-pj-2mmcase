---
name: gemm-hls-reference
description: Use this skill when borrowing high-level GEMM architecture ideas and adapting them carefully to the 100x100 short 2mm project.
---

# GEMM HLS Reference

## When to use

Use this skill only for architecture inspiration that can be adapted to this
specific 2mm case.

## Inputs to read

- `AGENTS.md`
- `designs/2mm/spec.json`
- `refs/repo_notes/GEMM-HLS-borrowed.md`
- Any local upstream GEMM-HLS notes or examples, if available.

## Required workflow

1. Identify the transferable architecture idea.
2. Remove assumptions tied to FP32, Alveo, wide AXI, URAM, or xclbin flow.
3. Recompute dimensions and buffering for 100x100 `short`.
4. Preserve internally generated matrix semantics and `sum` output.
5. Evaluate against XC7K325T resources and post-route timing.

## Hard constraints

- The part is fixed to `xc7k325tffv900-2`.
- Do not copy a GEMM-HLS implementation directly.
- Do not assume 32x32 FP32 arrays, Alveo U50, URAM, 512-bit AXI, `v++`, or
  300 MHz.

## Expected output

- Adapted idea summary.
- Risks and resource implications.
- Notes on what cannot be reused.

## What not to do

- Do not treat GEMM-HLS as drop-in source code.
- Do not change the 2mm external interface.
- Do not replace the fixed Vivado part.

