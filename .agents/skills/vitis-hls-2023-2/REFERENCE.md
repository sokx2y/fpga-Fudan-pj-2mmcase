# Vitis HLS 2023.2 Reference

Project target:

- Toolchain: Vitis HLS 2023.2.
- Top function: `kernel_2mm`.
- Fixed Vivado part: `xc7k325tffv900-2`.

Use `designs/2mm/scripts/run_hls.tcl` as the flow template. The script accepts
environment overrides for top function, source files, testbench files, clock
period, variant name, cosimulation, and export. The part is not configurable.

Required default stages:

- `csim_design`
- `csynth_design`

Optional stages:

- `cosim_design` when `HLS_RUN_COSIM` is true.
- `export_design -format ip_catalog` when `HLS_EXPORT_RTL` is true.

Forbidden assumptions:

- No 2025.2-only APIs.
- No Alveo platform flow.
- No `v++` or xclbin flow.
- No automatic part fallback.

