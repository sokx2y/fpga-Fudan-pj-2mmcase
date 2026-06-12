# Optimization Objective

This document is the canonical source of truth for the 2mm optimization goal.

## Final Objective

Final objective: minimize `latency_cycles * post_route_clock_period` for a
correct, resource-fitting, timing-valid implementation on
`xc7k325tffv900-2`.

The Xeon CPU baseline is 4.632 ms. The final FPGA result must report runtime
and speedup against that baseline.

## Hard Constraints

Candidate designs must satisfy these constraints before final ranking:

1. C simulation correctness passes.
2. RTL behavior is validated for shortlisted designs.
3. HLS synthesis succeeds.
4. Vivado synthesis succeeds.
5. Vivado implementation, place, and route succeed.
6. Resource usage fits `xc7k325tffv900-2`.
7. Timing information is valid and reportable.

## Primary Metric

The final performance metric is:

```text
T_run = latency_cycles * post_route_clock_period
```

or equivalently:

```text
T_run = latency_cycles / Fmax_post_route
```

HLS latency alone is not the final objective. HLS estimated clock period is an
early estimate only. Final ranking must use implemented timing from Vivado.

## HLS Versus Vivado

HLS C simulation proves functional behavior at the C level.

HLS C synthesis is useful for broad architecture and clock-constraint
exploration. It reports latency cycles, II, estimated clock, and estimated
resources, but it does not prove final implementation speed.

RTL cosimulation should be run on shortlisted designs before treating generated
RTL as behaviorally validated.

Vivado synthesis and place-and-route provide routed resources, WNS, critical
path period or achieved Fmax, and implementation success/failure. These reports
drive final performance ranking.

## Coupled DSE Dimensions

Architecture parameters and clock parameters must be explored together.

Architecture examples:

- `DOT_UNROLL_FACTOR`
- `OUT_UNROLL_FACTOR`
- full D storage versus on-the-fly short D reduction
- full tmp storage versus tmp row or tile buffering
- pipeline placement
- DATAFLOW structure
- buffering, banking, fusion, and PE-array candidates

Clock examples:

- 10.0 ns
- 8.0 ns
- 7.0 ns
- 6.5 ns
- 6.0 ns
- additional values around the implementation boundary

Changing the HLS clock constraint can change scheduling, operator chaining,
latency cycles, resources, and routing difficulty. Do not assume one fixed
10 ns HLS sweep determines the final winner.

## Correct Stage Interpretation

- Stage 0 establishes correctness and a conservative HLS baseline.
- Stage 1 reduces HLS latency cycles through inner-product parallelism.
- Stage 2 explores DOT and OUT parallelism at an initial 10 ns HLS constraint.
- Stage 2.5 co-explores architecture and clock constraints, then validates
  shortlisted candidates with RTL cosimulation, RTL export, and Vivado
  implementation.
- Stage 3 introduces structural optimizations such as on-the-fly D reduction,
  tmp row buffering, fusion, ping-pong buffering, and DATAFLOW.
- Final selection ranks timing-clean, resource-fitting implementations by
  `latency_cycles * post_route_clock_period`.

Stage 2 may identify an HLS latency winner. It does not identify the final FPGA
winner until Vivado implementation results exist.

## Required Candidate Result Fields

Each serious candidate should record:

- `variant_name`
- architecture parameters
- HLS clock constraint
- C simulation pass/fail
- expected sum
- actual sum
- HLS latency min/max cycles
- HLS interval / II
- HLS estimated clock period
- HLS estimated resources
- RTL cosimulation pass/fail
- Vivado synthesis pass/fail
- Vivado implementation pass/fail
- requested clock period
- post-synthesis WNS
- post-route WNS
- routed critical-path period or achieved Fmax
- routed DSP, BRAM, LUT, and FF
- final FPGA runtime
- speedup versus CPU 4.632 ms
- failure reason
- warning summary
- final ranking status

Use precise ranking labels:

- `hls_latency_winner`
- `hls_estimated_runtime_winner`
- `post_route_runtime_winner`

Avoid the generic label `best_candidate` unless the stage and metric are named.
