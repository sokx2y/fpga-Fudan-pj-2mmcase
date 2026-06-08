# HLS 2mm Architecture DSE Reference

Project target:

- Fixed Vivado part: `xc7k325tffv900-2`.
- Final metric: `latency_cycles * post_route_clock_period_ns`.

Candidate variants:

- baseline
- pipeline
- k-unroll factor 5, 10, 20, or 25
- sum-only no-D-store
- C_colsum transform
- tmp tile buffer
- tile-fused GEMM1/GEMM2 communication
- fine-grained dataflow pipeline
- systolic / PE array candidate later

DSE rules:

- Start with manual, controlled variants.
- Record correctness, latency, clock, resources, and report paths.
- Use Vivado implementation results for final clock.
- Do not launch complex multi-agent exploration in the first stage.
- Do not substitute the fixed part with another package.

