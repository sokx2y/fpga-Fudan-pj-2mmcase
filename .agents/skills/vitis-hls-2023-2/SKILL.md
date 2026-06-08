---
name: vitis-hls-2023-2
description: Use this skill when preparing or running Vitis HLS 2023.2 C simulation, synthesis, optional cosimulation, or RTL export for the 2mm project.
---

# Vitis HLS 2023.2

## When to use

Use this skill for HLS flow setup, script review, or report collection for the
PolyBench 2mm design.

## Inputs to read

- `AGENTS.md`
- `designs/2mm/spec.json`
- `designs/2mm/scripts/check_env.tcl`
- `designs/2mm/scripts/run_hls.tcl`
- The variant source and testbench paths selected by environment variables.

## Required workflow

1. Confirm the fixed Vivado part is `xc7k325tffv900-2`.
2. Check that the HLS flow uses Vivado/Vitis HLS 2023.2-compatible commands.
3. Run or review `csim_design` and `csynth_design`.
4. Run `cosim_design` only when explicitly enabled.
5. Run `export_design` only when explicitly enabled.
6. Record source paths, clock period, part, variant name, and report paths.

## Hard constraints

- The part is fixed to `xc7k325tffv900-2`.
- Do not introduce another package as a candidate, default, or fallback.
- Do not use 2025.2-only APIs such as `hls::task` or `hls::stream_of_blocks`.
- Do not use Alveo, U50, `v++`, xclbin, HBM, or URAM assumptions.

## Expected output

- Clear HLS run commands or script edits.
- C simulation status.
- C synthesis status.
- Report locations and summary fields needed by `parse_reports.py`.

## What not to do

- Do not implement `kernel_2mm_baseline.cpp` as part of skill repair.
- Do not create optimization variants.
- Do not replace the fixed part.

