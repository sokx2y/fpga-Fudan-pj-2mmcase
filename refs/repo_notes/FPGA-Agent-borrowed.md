# FPGA-Agent Borrowed Ideas

This project borrows workflow organization ideas from FPGA-Agent, not code or
tool-version assumptions.

## Borrowed

- Skill three-layer structure:
  `SKILL.md`, `REFERENCE.md`, and `examples/`.
- Clear HLS and Vivado flow separation.
- DSE checklist thinking before and after each variant.
- Report-driven optimization using synthesis, cosimulation, implementation,
  timing, utilization, and Pareto-style summaries.

## Not Borrowed

- No 2025.2-only APIs.
- No `hls::task`.
- No `hls::stream_of_blocks`.
- No Alveo U50 assumption.
- No `v++` or xclbin platform flow.
- No HBM or URAM assumption.
- No automatic target-device fallback.

## Local Adaptation

All project-local skills must target Vivado/Vitis HLS 2023.2 and the fixed
Vivado part `xc7k325tffv900-2`. Final performance must be based on
post-route clock period, not HLS estimates alone.

