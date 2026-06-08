---
name: hls-2mm-architecture-dse
description: Use this skill when planning controlled 2mm architecture design-space exploration variants and report-driven comparisons.
---

# HLS 2mm Architecture DSE

## When to use

Use this skill after the baseline exists and the user asks to plan or compare
architecture variants.

## Inputs to read

- `AGENTS.md`
- `designs/2mm/spec.json`
- Baseline source and reports.
- Existing variant metadata and reports.

## Required workflow

1. Start from a known-correct baseline.
2. Pick one controlled variant idea at a time.
3. Record expected memory, compute, and timing pressure.
4. Run correctness before trusting performance data.
5. Use HLS reports for early estimates.
6. Use Vivado post-route clock for final metric.
7. Record latency, clock, resources, correctness, and report paths.

## Hard constraints

- The part is fixed to `xc7k325tffv900-2`.
- Final metric is `latency_cycles * post_route_clock_period_ns`.
- First-stage DSE must be manual and controlled.
- Do not assume 2025.2 APIs or Alveo-style platform flow.

## Expected output

- Variant plan.
- Report-backed comparison table.
- Clear recommendation for the next controlled experiment.

## What not to do

- Do not create variants during skeleton repair.
- Do not launch complex multi-agent exploration in the first stage.
- Do not copy GEMM-HLS designs directly.

