# Vivado Implementation 2023.2 Reference

Project target:

- Toolchain: Vivado 2023.2.
- Fixed Vivado part: `xc7k325tffv900-2`.
- Timing must pass with WNS >= 0.

Use ordinary Vivado project or non-project implementation flow only:

- `synth_design`
- `opt_design`
- `place_design`
- `phys_opt_design`
- `route_design`
- `report_timing_summary`
- `report_utilization`

The implementation script must keep the part fixed to `xc7k325tffv900-2`. It
may not substitute another package.

