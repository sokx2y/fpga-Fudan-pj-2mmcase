---
name: hls-2mm-architecture-dse
description: Use this skill when planning controlled 2mm architecture DSE with FPGA-Agent-style architect records and report-driven comparison.
---

# HLS 2mm Architecture DSE

## When to use

Use this skill after the project skeleton is ready and the user asks to plan,
rank, or compare architecture candidates. It may be used before the baseline
exists for planning only, but it must not create kernel code by itself.

## Inputs to read

- `AGENTS.md`
- `designs/2mm/spec.json`
- `refs/method_library/fpga_agent_method_map.md`
- `refs/method_library/gemm_hls_method_map.md`
- `refs/method_library/2mm_architecture_catalog.md`
- Baseline or variant reports when they exist.

## Required workflow

1. Choose one candidate from the architecture catalog.
2. Produce an FPGA-Agent-style Architect six-element record.
3. Classify the candidate as Explorer, Exploiter, or Innovator.
4. Fill in hardware structure, dataflow diagram, storage hierarchy,
   GEMM1/GEMM2 communication, critical path estimate, resource scaling model,
   expected latency formula, correctness risk, when to try, and when to reject.
5. Run the T1 hardware checklist before allowing HLS synthesis.
6. Use HLS reports for early estimates only.
7. Use Vivado post-route clock for final metric.

## Hard constraints

- The part is fixed to `xc7k325tffv900-2`.
- Final metric is `latency_cycles * post_route_clock_period_ns`.
- DSE must be manual, controlled, and report-driven.
- Do not assume 2025.2 APIs, Alveo, U50, HBM, URAM, `v++`, xclbin, or
  512-bit AXI.
- Do not implement a kernel from this skill unless the user explicitly asks for
  implementation in a later task.

## Expected output

- Architecture decision record.
- T1 checklist result.
- Proceed/reject recommendation.
- Report-backed comparison table when reports exist.

## What not to do

- Do not create variants during method migration.
- Do not launch complex multi-agent exploration in the first stage.
- Do not copy GEMM-HLS source code directly.
- Do not use `scaled_systolic_pe_array` as the baseline.

