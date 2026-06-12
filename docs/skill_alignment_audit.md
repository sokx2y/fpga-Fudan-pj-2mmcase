# Skill Alignment Audit

Audit date: 2026-06-09

Canonical objective:

```text
Final objective: minimize latency_cycles * post_route_clock_period for a correct, resource-fitting, timing-valid implementation on xc7k325tffv900-2.
```

## Summary

The project was mostly aligned at the top-level method layer: `AGENTS.md`,
`designs/2mm/spec.json`, and `refs/method_library/` already mentioned
post-route clock as the final performance clock. The main objective drift was
in v2 sweep wording and generated summaries, which described the lowest-HLS-
cycle candidate as the "best" or the recommended next point without making
clear that it is only an HLS-stage winner.

The repaired convention is:

- HLS C simulation and C synthesis are screening stages.
- HLS latency cycles and HLS estimated clock are not final performance.
- Stage 2 identifies `hls_latency_winner` and `hls_estimated_runtime_winner`.
- Stage 2.5 must run RTL cosimulation, RTL export, Vivado synthesis, and Vivado
  place-and-route on shortlisted architecture and clock combinations.
- Final ranking uses `latency_cycles * post_route_clock_period_ns`.

## Files Inspected

| Path | Role | Alignment | Drift Found | Severity | Correction |
| --- | --- | --- | --- | --- | --- |
| `AGENTS.md` | Top-level project rules | Partially aligned | Final metric existed, but variant records did not explicitly require RTL/Vivado/final runtime fields. | Medium | Updated to require post-route final ranking fields and clock as a design variable. |
| `designs/2mm/spec.json` | Machine-readable project facts | Partially aligned | Final metric existed, but no ranking labels or clock/architecture DSE dimensions. | Medium | Added ranking labels and architecture/clock DSE dimensions. |
| `.agents/skills/vitis-hls-2023-2/SKILL.md` | HLS flow skill | Partially aligned | Records HLS outputs but does not explicitly say HLS winners are screening-only. | Medium | Proposed: add HLS screening language and forbid calling HLS-only winner final FPGA winner. Current workspace permissions prevented writing `.agents/skills`. |
| `.agents/skills/vitis-hls-2023-2/REFERENCE.md` | HLS flow reference | Partially aligned | Does not explicitly distinguish HLS latency winner from post-route winner. | Medium | Proposed: add `hls_latency_winner`, `hls_estimated_runtime_winner`, and final post-route metric note. |
| `.agents/skills/vivado-impl-2023-2/SKILL.md` | Vivado implementation skill | Aligned | Already says not to infer final performance from HLS estimates alone. Missing explicit final runtime fields. | Low | Proposed: require requested clock, WNS, routed critical path/Fmax, routed resources, runtime, and speedup. |
| `.agents/skills/vivado-impl-2023-2/REFERENCE.md` | Vivado reference | Partially aligned | Lists flow commands but not final-results schema. | Low | Proposed: add final implementation comparison fields. |
| `.agents/skills/hls-2mm-intake/*` | Project facts intake | Aligned | No ranking drift; correctly records CPU baseline and fixed part. | Low | No change required. |
| `.agents/skills/hls-2mm-baseline/*` | Correctness-first baseline | Aligned | Baseline is C-sim/HLS focused, not final ranking. | Low | No change required. |
| `.agents/skills/hls-2mm-checklist/*` | Variant hardware checklist | Partially aligned | Mentions post-route clock, but does not explicitly require ranking-label separation. | Medium | Proposed: add HLS-vs-post-route winner distinction and final runtime fields. |
| `.agents/skills/hls-2mm-architecture-dse/*` | Architecture DSE method | Partially aligned | Correct final metric, but clock co-exploration and shortlist-to-Vivado workflow were not explicit enough. | Medium | Proposed: add architecture x clock DSE workflow and ranking labels. |
| `.agents/skills/fpga-agent-method-port/*` | FPGA-Agent method migration | Aligned | T3/T4 were described as deferred/optional until reports exist; final ranking gate could be sharper. | Low | Proposed: clarify T3/T4 are required for final ranking of shortlisted designs. |
| `.agents/skills/gemm-hls-reference/*` | GEMM-HLS porting guide | Mostly aligned | Good platform constraints; final post-route ranking not explicit. | Low | Proposed: add final metric reminder when borrowing PE/tile structures. |
| `.agents/skills/gemm-hls-method-port/*` | GEMM-HLS method port | Mostly aligned | Same as above. | Low | Proposed: add implemented runtime ranking reminder. |
| `.agents/gateflow_agents/` | Agent workflows | Not present | Directory did not exist or contained no files in this checkout. | None | No change. |
| `refs/method_library/fpga_agent_method_map.md` | FPGA-Agent method map | Mostly aligned | Post-route metric existed, but HLS winner terminology was not explicit. | Low | Updated with HLS winner vs post-route winner terminology and clock co-exploration reminder. |
| `refs/method_library/2mm_architecture_catalog.md` | Architecture catalog | Mostly aligned | Final metric existed, but catalog latency models could be read as final. | Low | Updated to state HLS latency models are screening estimates only. |
| `refs/method_library/gemm_hls_method_map.md` | GEMM-HLS method map | Partially aligned | Architecture borrowing did not explicitly connect to final post-route runtime ranking. | Low | Updated with implemented runtime ranking rule. |
| `designs/2mm/scripts/run_hls.tcl` | HLS template | Aligned | Supports `HLS_CLOCK_PERIOD_NS`; no final-ranking logic. | Low | No change required. |
| `designs/2mm/scripts/run_vivado_impl.tcl` | Vivado implementation template | Partially aligned | Produced only generic timing/utilization reports after route. | Medium | Updated to write post-synthesis and post-route timing/utilization reports. |
| `designs/2mm/scripts/parse_reports.py` | HLS/Vivado report summarizer | Partially aligned | Parsed reports but did not compute final runtime or speedup. | High | Updated to emit `final_metrics`, routed critical path/Fmax, and speedup versus 4.632 ms when reports exist. |
| `variants/README.md` | Variant result conventions | Missing | No central variant schema or ranking vocabulary. | Medium | Created with serious-candidate fields and ranking labels. |
| `variants/v0_baseline/*` | Stage 0 record | Mostly aligned | Correctly says final runtime later needs post-route clock. | Low | No change. |
| `variants/v1_dotprod_unroll10/*` | Stage 1 record | Partially aligned | HLS-focused wording is acceptable for Stage 1; should not be treated as final. | Low | No change; global docs now clarify. |
| `variants/v2_parallel_boundary_sweep/notes.md` | Stage 2 notes | Misaligned | Called fastest passing latency preferred, without final-stage qualification. | High | Updated to define Stage 2 as HLS parallelism sweep and require Stage 2.5. |
| `variants/v2_parallel_boundary_sweep/architecture_decision.md` | Stage 2 architecture record | Misaligned | Boundary questions used generic "best latency" wording and did not force Vivado validation. | High | Updated with HLS winner terms and Stage 2.5 shortlist/clock sweep. |
| `variants/v2_parallel_boundary_sweep/how_to_run.md` | Stage 2 run instructions | Misaligned | Parser output described "best latency candidate" and "recommended next architecture" without final-ranking caveat. | High | Updated to HLS-stage labels and post-route final metric. |
| `variants/v2_parallel_boundary_sweep/scripts/parse_hls_reports.py` | v2 sweep parser | Misaligned | Generated `Best Latency Candidate` and recommendation language; schema lacked post-route placeholders. | High | Updated summary labels and added post-route/final-runtime placeholder columns. |
| `variants/v2_parallel_boundary_sweep/sweep_summary.md` | Generated v2 summary | Misaligned | Could be read as selecting final winner. | High | Regenerated with HLS-stage labels and Stage 2.5 recommendation. |
| `variants/v2_parallel_boundary_sweep/sweep_results.csv` | Generated v2 result table | Partially aligned | HLS-only fields; no post-route/final-runtime columns. | Medium | Regenerated from existing reports with HLS estimated runtime and post-route placeholder columns. |

## Misalignments Found

1. v2 summary and parser ranked only by HLS latency cycles.
2. v2 wording used generic "best" language.
3. HLS estimated clock was treated as a pass filter, but final post-route timing
   was not carried into the generated schema.
4. The Stage 2 plan did not clearly require architecture x clock
   co-exploration.
5. The final-results schema was missing routed WNS, routed critical path,
   achieved Fmax, final runtime, and speedup fields.
6. Some skills were conceptually aligned but did not state the ranking
   distinction strongly enough.

## Corrected Evaluation Rules

- Do not rank final designs by HLS latency cycles alone.
- Do not treat HLS estimated clock as final clock.
- Use HLS sweeps to shortlist candidates, not to select the final FPGA winner.
- Explore architecture parameters and HLS clock constraints together.
- Run RTL cosimulation and Vivado implementation on shortlisted candidates.
- Use routed timing and routed resources for final comparison.
- Compute speedup against the 4.632 ms CPU baseline only after final runtime is
  known.

## Stage 2.5 Minimum Shortlist

Start with:

- `v2_dot25_out4_fullD`
- `v2_dot50_out4_fullD`
- `v2_dot100_out4_fullD`

Evaluate clock constraints such as:

- 10.0 ns
- 8.0 ns
- 7.0 ns
- 6.5 ns
- 6.0 ns

Then run RTL cosimulation, RTL export, Vivado synthesis, and Vivado
place-and-route for viable points.

## Remaining Gaps Before Stage 2.5

- Project-local `.agents/skills/*` files are read-only in the current sandbox,
  so their corrections are recorded here as proposed rather than applied.
- No RTL cosimulation has been run for v2 shortlisted candidates.
- No RTL export has been run for v2 shortlisted candidates.
- No Vivado implementation or post-route timing exists yet.
- The v2 generated HLS project directories contain HLS reports only; they are
  not final implementation evidence.
