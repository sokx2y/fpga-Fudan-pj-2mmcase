# v3i_dot25_out4_initless_row_pingpong_dsp_pipeline

## Intent

XC7K325T hardware-cooperative timing-relief variant based on the v3g champion.

This is not an algorithmic rewrite. It keeps:

- DOT25 / OUT4
- initless A/B/C
- row ping-pong DATAFLOW
- no full tmp
- no full D
- lane_sum reduction

## Local Changes From v3g

- Bind the 16-bit multiply result to DSP with explicit latency:
  `#pragma HLS bind_op variable=product op=mul impl=dsp latency=2`
- Fully partition `tmp_ping`, `tmp_pong`, and the row-buffer formal argument.
- Preserve short truncation by casting each product and accumulation back to
  `DATA_TYPE`.

## Hypothesis

Use currently spare FF/LUT fabric to ease DSP input/output timing and local row
buffer read pressure while preserving v3g-level latency.

Expected success signature:

- C-sim PASS, seed=3, expected_sum=957248
- HLS latency close to v3g 4539 cycles
- DSP remains close to 800
- Vivado post-route WNS improves versus v3g at tight clock points

Reject if:

- HLS latency rises materially above v3g
- DSP drops far below 800, indicating lost parallelism
- Vivado route congestion becomes worse than v3g

## Source

`designs/2mm/src/kernel_2mm_stage3i_dot25_out4_initless_row_pingpong_dsp_pipeline.cpp`

## Result: solution4_75 + Vivado 4.8 ns

HLS `solution4_75`:

- Estimated clock: 3.442 ns
- Latency: 4238 cycles
- `compute_tmp_row`: 36 cycles
- `consume_tmp_row`: 40 cycles
- DSP: 800 / 840
- BRAM_18K: 0 / 890
- FF: 43228 / 407600
- LUT: 49237 / 203800

Vivado implementation at 4.8 ns:

- Routed successfully
- WNS: +0.179 ns
- TNS: 0.000 ns
- WHS: +0.076 ns
- THS: 0.000 ns
- Route status: 56224 / 56224 routable nets fully routed, 0 routing errors
- Placed utilization: LUT 22978, FF 16506, BRAM tile 0, DSP 800

Runtime:

- Conservative at 4.8 ns constraint: `4238 * 4.800 = 20342.4 ns`
- Inferred from WNS: `4238 * (4.800 - 0.179) = 19584.398 ns`

Archived Vivado checkpoint:

`designs/2mm/hls_proj/v3i_dot25_out4_initless_row_pingpong_dsp_pipeline/vivado_impl_4p8_pass_20260611`

## Result: same HLS checkpoint + Vivado 4.6 ns

HLS `solution4_75` remains the architectural source of record:

- Estimated clock: 3.442 ns
- Latency: 4238 cycles
- `compute_tmp_row`: 36 cycles
- `consume_tmp_row`: 40 cycles
- DSP: 800 / 840
- BRAM_18K: 0 / 890

Vivado implementation at 4.6 ns:

- Routed successfully
- WNS: +0.069 ns
- TNS: 0.000 ns
- WHS: +0.082 ns
- THS: 0.000 ns
- Route status: 56173 / 56173 routable nets fully routed, 0 routing errors
- Placed utilization: LUT 22979, FF 16509, BRAM tile 0, DSP 800

Runtime:

- Conservative at 4.6 ns constraint: `4238 * 4.600 = 19494.8 ns`
- Speedup versus 4.632 ms CPU baseline: `4632000 / 19494.8 = 237.60x`
- WNS-inferred reference only: `4238 * (4.600 - 0.069) = 19202.378 ns`

DRC/methodology notes:

- DRC has `NSTD-1` and `UCIO-1` critical warnings from missing board-level I/O standard and LOC constraints.
- DRC also has `CFGBVS-1` and 256 `DPOP-2` warnings.
- Methodology has 54 `TIMING-18` warnings from missing input/output delays.
- No routed `TIMING-17` non-clocked sequential-cell issue was observed.

Archived Vivado checkpoint:

`designs/2mm/hls_proj/v3i_dot25_out4_initless_row_pingpong_dsp_pipeline/vivado_impl_4p6_pass_20260611`

Interpretation:

This is the current best recorded checkpoint. The worst setup path is now route-dominated from `ap_CS_fsm_reg[2]_replica/C` to a `tmp_ping` clock-enable register, with 4.536 ns data path delay, 4.313 ns of route delay, and 0 logic levels. Further gains are likely to depend on physical/control-distribution relief or Vivado implementation DSE, not only arithmetic pipeline changes.
