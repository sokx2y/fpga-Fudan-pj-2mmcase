# Vitis HLS 2023.2 Reference

Project target:

- Toolchain: Vitis HLS 2023.2.
- Top function: `kernel_2mm`.
- Fixed Vivado part: `xc7k325tffv900-2`.

Use `designs/2mm/scripts/run_hls.tcl` as the flow template. The script accepts
environment overrides for top function, source files, testbench files, clock
period, and variant name, but the part is a hard constraint. If a different part
is requested, the flow must fail immediately.

Required stages:

- `csim_design`
- `csynth_design`
- `cosim_design`
- `export_design -format ip_catalog`

Forbidden assumptions:

- No 2025.2-only APIs.
- No Alveo platform flow.
- No `v++` or xclbin flow.
- No automatic part fallback.

