---
name: vivado-impl-2023-2
description: Use this skill when preparing or running Vivado 2023.2 synthesis, implementation, timing, and utilization checks for exported 2mm RTL.
---

# Vivado Implementation 2023.2

## When to use

Use this skill after HLS has exported RTL and the project needs post-route timing
or utilization data.

## Inputs to read

- `AGENTS.md`
- `designs/2mm/spec.json`
- `designs/2mm/scripts/check_env.tcl`
- `designs/2mm/scripts/run_vivado_impl.tcl`
- Exported RTL under the selected `RTL_PATH`.

## Required workflow

1. Confirm Vivado recognizes `xc7k325tffv900-2`.
2. Read RTL from `RTL_PATH`.
3. Create or review a Vivado project using the fixed part.
4. Run synthesis, opt, place, phys-opt, and route.
5. Generate timing and utilization reports.
6. Fail the flow when WNS is negative.

## Hard constraints

- The part is fixed to `xc7k325tffv900-2`.
- WNS must be greater than or equal to zero for a passing implementation.
- Do not assume Alveo, U50, HBM, URAM, or xclbin flow.

## Expected output

- Timing report path.
- Utilization report path.
- WNS result.
- Resource summary fields for DSP, BRAM, LUT, and FF.

## What not to do

- Do not use another package as fallback.
- Do not infer final performance from HLS estimates alone.
- Do not modify the algorithm while repairing implementation flow scripts.

