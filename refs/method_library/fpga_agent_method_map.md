# FPGA-Agent Method Map For 2mm

This note ports method-level ideas from FPGA-Agent into this repository. It does
not copy source code and does not import tool assumptions that conflict with the
local project.

## Sources Consulted

- `FPGA-Agent/README.md`
- `FPGA-Agent/DSE-agent/agent.md`
- `FPGA-Agent/DSE-agent/prompts/architect.md`
- `FPGA-Agent/DSE-agent/prompts/explorer.md`
- `FPGA-Agent/DSE-agent/prompts/exploiter.md`
- `FPGA-Agent/DSE-agent/prompts/innovator.md`
- `FPGA-Agent/DSE-agent/prompts/hardware_checklist.md`
- `FPGA-Agent/DSE-agent/prompts/coding_style.md`
- `FPGA-Agent/vitis-hls-synthesis/SKILL.md`
- `FPGA-Agent/vitis-hls-synthesis/REFERENCE.md`
- `FPGA-Agent/vivado-impl/SKILL.md`
- `FPGA-Agent/vivado-analysis/SKILL.md`

The local clone failed because GitHub was unreachable from this environment, so
the method pass used GitHub raw source views where available.

## Original Method Points

FPGA-Agent separates hardware design exploration into a method layer and tool
flow layer:

- A three-layer skill structure: `SKILL.md`, `REFERENCE.md`, and `examples/`.
- Architect first: describe the candidate architecture before editing code.
- Worker selection: Explorer, Exploiter, or Innovator depending on risk and
  novelty.
- Tiered validation:
  T1 checklist -> T2 synthesis -> T3 cosimulation -> T4 implementation.
- Hardware checklist before expensive tool runs.
- Report-driven optimization rather than intuition-only tuning.
- Coding style that prioritizes macro-architecture, then micro-pipelines, then
  parameter tuning.
- Load/compute/store or dataflow stage decomposition before applying pragmas.

## Local Migration

For this 2mm project, the method layer is migrated as follows:

| FPGA-Agent method | 2mm migration |
| --- | --- |
| Three-layer skill layout | All project-local skills keep `SKILL.md`, `REFERENCE.md`, and `examples/`. |
| Architect pass | Every architecture candidate must produce six design elements before coding. |
| Explorer worker | Used for broad but bounded variants such as no-D-store or C-colsum. |
| Exploiter worker | Used for local improvements such as unroll factors and partition factors. |
| Innovator worker | Reserved for fused tmp communication or scaled PE-array concepts. |
| T1 checklist | Run method-level hardware checklist before HLS. |
| T2 synthesis | Use HLS synthesis only after T1 passes. |
| T3 cosim | Enable only when source and testbench exist and correctness is meaningful. |
| T4 implementation | Use Vivado post-route timing for final `runtime_ns`. |
| Report-driven comparison | Store latency, clock, utilization, WNS, correctness, and report paths. |

HLS-only sweeps may rank `hls_latency_winner` or
`hls_estimated_runtime_winner`. The final project winner must be a
`post_route_runtime_winner` selected from RTL-validated, timing-clean,
resource-fitting Vivado implementations.

## Architect Six Elements

Each architecture candidate must include:

1. Hardware structure.
2. Dataflow diagram.
3. Storage hierarchy.
4. GEMM1/GEMM2 communication path.
5. Critical-path estimate.
6. Resource and latency model.

For this repository, each candidate also records correctness risk and reject
conditions because 2mm uses internally generated `short` matrices and a
`sum`-only output.

## Worker Selection

- Explorer: try structurally different candidates with controlled scope.
  Examples: `sum_only_no_D_store`, `c_colsum_reduced_second_gemm`,
  `tmp_tile_buffer_fused_gemm`.
- Exploiter: tune an already working structure.
  Examples: k-unroll factor, array partition factor, tile size.
- Innovator: introduce a more novel communication or scheduling structure.
  Examples: row-stationary tile pipeline, wavefront PE schedule, scaled
  systolic candidate.

## Reusable Checklist Items

The following checklist items can be reused directly:

- Pipeline placement and II target.
- Unroll factor matched with array partitioning.
- BRAM port pressure.
- DATAFLOW single-producer/single-consumer structure.
- FIFO depth and deadlock risk.
- Reduction tree depth.
- Critical-path estimate before synthesis.
- Post-synthesis consistency check against expected structure.
- Post-route WNS check before accepting final performance.

## Tool Commands That Need Rewriting

Any upstream command that assumes newer tools or platform compilation must be
rewritten:

- Replace 2025.2-only HLS APIs with Vitis HLS 2023.2-compatible code.
- Replace Alveo platform flow with plain HLS and Vivado project/non-project
  flow.
- Replace `v++` and xclbin build steps with `csim_design`, `csynth_design`,
  optional `cosim_design`, optional `export_design`, and Vivado implementation.
- Replace device defaults with fixed part `xc7k325tffv900-2`.
- Replace HBM/URAM storage assumptions with BRAM/register/LUTRAM candidates
  validated against XC7K325T.

## Non-Migrated Content And Reasons

- 2025.2-only APIs: incompatible with Vivado/Vitis HLS 2023.2.
- Alveo U50 platform commands: this project targets Kintex-7
  `xc7k325tffv900-2`.
- HBM or URAM assumptions: not valid as defaults for this target.
- `v++` and xclbin flow: not part of the local flow.
- Any automatic part fallback: the part is fixed and must fail fast if missing.

## Effect On 2mm DSE

This method map turns 2mm exploration into a staged decision process:

1. Produce an architecture decision record.
2. Classify the change as Explorer, Exploiter, or Innovator.
3. Run the T1 hardware checklist.
4. Only then create source changes or run HLS.
5. Treat Vivado implementation clock as the final performance clock.
6. Explore architecture and clock constraints together for shortlisted
   candidates before final ranking.
