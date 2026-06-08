# Project Rules

This repository is for the PolyBench 2mm HLS case targeting Vivado/Vitis HLS
2023.2.

## Fixed Tool And Device

- Toolchain: Vivado/Vitis HLS 2023.2.
- FPGA family target: XC7K325T speed grade -2.
- Vivado part is fixed to `xc7k325tffv900-2`.
- Do not use alternate packages as candidates, defaults, or automatic fallbacks.
- `check_env.tcl` may use `get_parts *xc7k325t*900*` only to verify whether
  Vivado recognizes the fixed part. If the fixed part is not available, fail
  fast and print the matching available part list.

## Compatibility Boundaries

- Use syntax and flow compatible with Vivado/Vitis HLS 2023.2.
- Do not use 2025.2-only HLS APIs such as `hls::task` or
  `hls::stream_of_blocks`.
- Do not assume Alveo, U50, `v++`, xclbin, HBM, or URAM.
- Final performance metric is:
  `runtime_ns = latency_cycles * post_route_clock_period_ns`.

## Source Layout

- `refs/original_polybench/` is archival material and must not be edited.
- `refs/old_hls_autotb/` is old generated RTL testbench reference only.
- New HLS source belongs under `designs/2mm/src/`.
- New C++ testbench source belongs under `designs/2mm/tb/`.
- New flow scripts belong under `designs/2mm/scripts/`.
- Optimization variants belong under `designs/2mm/variants/<variant_name>/`.
- Each variant must record correctness, latency, clock, and resource data.

