# Final Results Template

Use this table for Stage 2.5 and final-stage comparisons. Leave fields blank or
`pending` until the corresponding flow has actually run.

| variant_name | arch_params | hls_clock_ns | csim | cosim | hls_latency_cycles | hls_est_clock_ns | hls_est_runtime_ms | vivado_synth | vivado_impl | requested_clock_ns | post_synth_wns_ns | post_route_wns_ns | routed_critical_path_ns | achieved_fmax_mhz | routed_DSP | routed_BRAM_18K | routed_LUT | routed_FF | final_runtime_ms | speedup_vs_cpu_4p632ms | ranking_status | failure_reason |
| --- | --- | ---: | --- | --- | ---: | ---: | ---: | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| v2_dot25_out4_fullD | DOT=25, OUT=4, fullD=1 | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | Stage 2.5 shortlist | pending |
| v2_dot50_out4_fullD | DOT=50, OUT=4, fullD=1 | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | Stage 2.5 shortlist | pending |
| v2_dot100_out4_fullD | DOT=100, OUT=4, fullD=1 | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | Stage 2.5 shortlist | pending |

Ranking status vocabulary:

- `hls_latency_winner`: lowest HLS latency cycles in a defined HLS-only sweep.
- `hls_estimated_runtime_winner`: lowest `HLS latency * HLS estimated clock`
  in a defined HLS-only sweep.
- `post_route_runtime_winner`: lowest `latency_cycles * post_route_clock_period`
  among correctness-validated, resource-fitting, timing-valid implementations.
- `failed_csim`, `failed_cosim`, `failed_hls_synthesis`,
  `failed_vivado_synthesis`, `failed_vivado_implementation`,
  `failed_timing`, `failed_resource_fit`, or `pending`.
