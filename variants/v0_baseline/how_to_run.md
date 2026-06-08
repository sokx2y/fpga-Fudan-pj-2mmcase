# How To Run v0_baseline Manually

The agent has not run Vitis HLS or Vivado for this variant. These steps are for
manual execution.

## 1. Enter The Project Directory

In PowerShell:

```powershell
cd E:\GPT_codex\fpga\2mm_case
```

## 2. Open Vitis HLS 2023.2 Command Shell

Use the Xilinx/Vitis 2023.2 command prompt from the Start menu, or source the
Vitis/Vivado 2023.2 environment in your shell. The exact setup depends on your
local installation.

After setup, confirm:

```powershell
vitis_hls -version
vivado -version
```

## 3. Run C Simulation

The current `run_hls.tcl` template runs C simulation followed by C synthesis.
Run it from the scripts directory so the default relative source paths resolve:

```powershell
cd E:\GPT_codex\fpga\2mm_case\designs\2mm\scripts
$env:HLS_VARIANT = "v0_baseline"
$env:HLS_TOP = "kernel_2mm"
$env:HLS_CLOCK_PERIOD_NS = "10.0"
$env:HLS_SRC_FILES = "../src/kernel_2mm_baseline.cpp ../src/kernel_2mm.h"
$env:HLS_TB_FILES = "../tb/tb_kernel_2mm.cpp"
$env:HLS_RUN_COSIM = "0"
$env:HLS_EXPORT_RTL = "0"
vitis_hls -f run_hls.tcl
```

For C simulation, check the console or C simulation log for:

```text
expected_sum = ...
actual_sum   = ...
PASS
```

If it prints `FAIL`, do not trust synthesis results.

## 4. Run C Synthesis

The command above also runs `csynth_design` after `csim_design`. Keep
`HLS_RUN_COSIM=0` and `HLS_EXPORT_RTL=0` for Stage 0 unless you explicitly want
to extend the run later.

## 5. Report Locations

When run from `designs/2mm/scripts`, the HLS project should be created under:

```text
designs/2mm/scripts/hls_v0_baseline/
```

Common locations to check:

```text
designs/2mm/scripts/hls_v0_baseline/solution1/csim/report/
designs/2mm/scripts/hls_v0_baseline/solution1/syn/report/
```

The exact filenames can vary, but the C synthesis report is usually similar to:

```text
kernel_2mm_csynth.rpt
```

## 6. Report Fields To Check

Check and record:

- C-sim PASS/FAIL.
- `expected_sum`.
- `actual_sum`.
- Latency cycles, min and max.
- Estimated clock period.
- DSP usage.
- BRAM usage.
- LUT usage.
- FF usage.
- Loop II values.

## 7. Fill Results Back Into variant.json

After manual HLS run, update:

```text
variants/v0_baseline/variant.json
```

Suggested fields:

- `correctness.c_sim`
- `correctness.expected_sum`
- `correctness.actual_sum`
- `correctness.status`
- `latency.cycles_min`
- `latency.cycles_max`
- `latency.estimated_clock_period_ns`
- `resources.DSP`
- `resources.BRAM`
- `resources.LUT`
- `resources.FF`
- `reports.csim_log`
- `reports.csynth_report`
- `decision`

Set `decision` to `csim_passed_csynth_available` only if C simulation passes and
C synthesis completes.

## 8. If C Simulation Fails

Check these first:

- `seed` is fixed to 3 in the testbench.
- The kernel and golden reference both generate A, B, C, D, and tmp internally.
- `tmp` and `D` are `short` matrices, not `int` matrices.
- The golden reference does not call `kernel_2mm`.
- The top signature is exactly `void kernel_2mm(short seed, int *sum)`.
- Include paths point to `designs/2mm/src/kernel_2mm.h`.

## 9. If C Synthesis Fails

Check these first:

- You are using Vitis HLS 2023.2.
- The part is still `xc7k325tffv900-2`.
- The command was run from `designs/2mm/scripts`.
- `HLS_SRC_FILES` and `HLS_TB_FILES` point to existing files.
- The source file contains no `main()`.
- No unsupported 2025.2-only APIs are present.
- Large local arrays may need later storage directives, but Stage 0 should not
  add optimization pragmas before correctness is established.
