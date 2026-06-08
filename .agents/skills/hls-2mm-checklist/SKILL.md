---
name: hls-2mm-checklist
description: Use this skill when checking a 2mm optimization idea or variant for memory bandwidth, pipeline, dataflow, timing, and resource risks.
---

# HLS 2mm Checklist

## When to use

Use this skill before and after each optimization variant is proposed or
evaluated.

## Inputs to read

- `AGENTS.md`
- `designs/2mm/spec.json`
- The variant source files.
- HLS and Vivado reports for the variant.

## Required workflow

1. Check loop pipelining choices.
2. Match unroll factors with memory partitioning and port availability.
3. Check BRAM, DSP, LUT, and FF use against XC7K325T capacity.
4. Review DATAFLOW producer/consumer structure.
5. Review FIFO depth and deadlock risk.
6. Review reduction depth and critical path risk.
7. Record correctness, latency, clock, and resources.

## Hard constraints

- The part is fixed to `xc7k325tffv900-2`.
- Use post-route clock for final performance.
- Do not assume URAM, HBM, Alveo, U50, or xclbin flow.

## Expected output

- Checklist result.
- Risks that block the next run.
- Report-backed notes for the variant.

## What not to do

- Do not approve a variant without correctness evidence.
- Do not hide timing failure behind HLS estimates.
- Do not use another Vivado package as capacity target.

