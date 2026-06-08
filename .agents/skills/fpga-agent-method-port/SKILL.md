---
name: fpga-agent-method-port
description: Use this skill before 2mm DSE to apply the FPGA-Agent architect, worker-selection, checklist, and tiered validation method.
---

# FPGA-Agent Method Port

## When to use

Use this skill before any 2mm architecture DSE step, before creating a variant,
or before running HLS synthesis for a proposed change.

## Inputs to read

- `AGENTS.md`
- `designs/2mm/spec.json`
- `refs/method_library/fpga_agent_method_map.md`
- `refs/method_library/2mm_architecture_catalog.md`
- Existing variant notes or reports, if any.

## Required workflow

1. Produce the Architect six-element description.
2. Classify the change as Explorer, Exploiter, or Innovator.
3. Run the T1 hardware checklist.
4. Decide whether the candidate is allowed to enter T2 HLS synthesis.
5. Record the decision as `architecture_decision.md` or
   `architecture_decision.json`.
6. Defer T3 cosimulation and T4 implementation until source and reports exist.

## Hard constraints

- The fixed Vivado part is `xc7k325tffv900-2`.
- Use Vivado/Vitis HLS 2023.2-compatible methods only.
- Do not use 2025.2-only APIs, Alveo, U50, HBM, URAM, `v++`, or xclbin flow.
- Do not implement kernel code from this skill.

## Expected output

- Architecture decision record.
- Worker classification.
- T1 checklist result.
- Clear proceed/reject decision for HLS synthesis.

## What not to do

- Do not skip the Architect pass.
- Do not run HLS before the T1 checklist passes.
- Do not treat HLS estimates as final performance.
- Do not change the target part.

