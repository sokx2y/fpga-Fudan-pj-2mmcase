# FPGA-Agent Method Port Reference

This skill ports FPGA-Agent methodology into the local 2mm project.

## Architect Six-Element Output

Each candidate must document:

1. Hardware structure.
2. Dataflow diagram.
3. Storage hierarchy.
4. GEMM1/GEMM2 communication path.
5. Critical-path estimate.
6. Resource and latency model.

The decision record must also include correctness risk and reject conditions.

## Worker Classes

- Explorer: structurally different but bounded experiments.
- Exploiter: tune an already working structure.
- Innovator: novel communication, scheduling, or PE-array ideas.

## Tiered Flow

- T1 checklist: method-level hardware review.
- T2 synthesis: HLS synthesis after T1 passes.
- T3 cosimulation: optional correctness validation when a testbench exists.
- T4 implementation: Vivado timing and utilization for final metrics.

## T1 Checklist

- Pipeline placement.
- Unroll and array partition consistency.
- BRAM port pressure.
- DATAFLOW producer/consumer ownership.
- FIFO depth and deadlock risk.
- Reduction tree depth.
- Critical-path estimate.
- Expected structure vs post-synthesis report.

## Fixed Project Constraints

- Part: `xc7k325tffv900-2`.
- Toolchain: Vivado/Vitis HLS 2023.2.
- Final metric: `latency_cycles * post_route_clock_period_ns`.

