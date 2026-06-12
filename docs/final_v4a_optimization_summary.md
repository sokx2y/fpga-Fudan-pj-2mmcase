# Final 2mm HLS Optimization Summary

Date: 2026-06-12

Target:

```text
Toolchain: Vitis HLS / Vivado 2023.2
Part: xc7k325tffv900-2
Problem: PolyBench 2mm, NI=NJ=NK=NL=100, DATA_TYPE=short
Top: kernel_2mm(short seed, int *sum)
CPU baseline: 4.632 ms
Final metric: latency_cycles * post_route_clock_period_ns
```

## Final Champion

```text
Variant: v4a_output_stationary_pe_cluster
Source: designs/2mm/src/kernel_2mm_stage4a_output_stationary_pe_cluster.cpp
Latency: 3434 cycles
Post-route clock: 4.500 ns
Routed WNS: +0.052 ns
Runtime: 15453.0 ns
Speedup vs CPU baseline: 299.75x
Resources: LUT 40981 / 203800, FF 30049 / 407600, DSP 840 / 840, BRAM 0 / 445
```

Final local Vivado archive:

```text
designs/2mm/variants/v4a_output_stationary_pe_cluster/vivado_4p50_20260612/
```

## Key Optimization Ideas

The most important optimization was removing dense matrix materialization and
initialization from the hardware datapath.

The original PolyBench-style algorithm initializes and stores dense matrices
`A`, `B`, `C`, `D`, and `tmp`.  For this benchmark instance, `A`, `B`, and `C`
are simple functions of loop indices and `seed`, while the only externally
observable output is the final integer sum.  The optimized design therefore
generates operands directly inside the compute loops and removes dense
initialization traffic from the critical hardware schedule.

The second major optimization was replacing full-matrix communication between
the two GEMM phases with row ping-pong communication.  Instead of storing all of
`tmp[100][100]`, the design computes one `tmp` row, consumes the previous row in
the second GEMM phase, and alternates two complete-partitioned row buffers.  This
preserves the required `short` intermediate semantics while avoiding full tmp
and D storage.

The final performance shape came from using a fully local dot-product engine:

```text
DOT_UNROLL_FACTOR = 20
OUT_UNROLL_FACTOR = 5
```

Each active compute or consume phase therefore exposes 100 short multiplications
per pipelined output tile, which maps to the full 840-DSP device after Vivado
implementation.  A local `ap_int<24>` accumulator preserves the intended short
product truncation while giving the adder tree enough width before the final
cast back to `short`.

V4A then rewrote the DOT20/OUT5 engine as explicit output-stationary PE
clusters.  It did not reduce cycles versus V3q, but it made the placement and
routing problem slightly more regular, allowing the same 3434-cycle latency to
close at 4.50 ns instead of 4.60 ns.

## Optimization Path

| Step | Main idea | Result / lesson |
| --- | --- | --- |
| Baseline style | Dense A/B/C/D/tmp arrays and original loop structure | Correct but far too much initialization/storage work. |
| Initless generation | Generate A/B/C operands from loop indices and seed | Most important algorithm-to-hardware cleanup; removed dense matrix initialization from the schedule. |
| Sum-only/no-D | Accumulate final sum directly instead of storing D | Reduced storage and final readback, while preserving externally visible behavior. |
| Row ping-pong | Keep only two `tmp_row[100]` buffers | Removed full tmp matrix and enabled GEMM1/GEMM2 overlap. |
| DOT25/OUT4 and DOT20/OUT5 sweeps | Increase parallel dot lanes and output lanes | DOT20/OUT5 became the best balance for cycles and routing. |
| DSP binding and local accumulation | Force short multiplies into DSPs and use local `ap_int<24>` accumulators | Preserved semantics and made DSP usage predictable. |
| V3q fanout relief | Local operand copies around the DOT20/OUT5 engine | Closed 4.60 ns with 3434 cycles, WNS +0.006 ns. |
| V4A output-stationary clusters | Make five output lanes explicit PE-style clusters | Same 3434 cycles, but routed at 4.50 ns, WNS +0.052 ns. |
| V4B systolic-like row-pair | Attempt more regular row-pair PE structure | Rejected: HLS latency 19534 cycles and only 200 DSP active. |
| V4C physical-control copies | Try to spend FF/LUT on C++-level local copies | Rejected: HLS optimized it back to the same summary as V4A. |

## Final Comparison

| Variant | Cycles | Routed clock | Runtime | Speedup | Notes |
| --- | ---: | ---: | ---: | ---: | --- |
| V3q DOT20/OUT5 fanout relief | 3434 | 4.600 ns | 15796.4 ns | 293.23x | Previous champion, WNS +0.006 ns. |
| V4A output-stationary cluster | 3434 | 4.550 ns | 15624.7 ns | 296.45x | First V4A routed pass, WNS +0.090 ns. |
| V4A output-stationary cluster | 3434 | 4.500 ns | 15453.0 ns | 299.75x | Final champion, WNS +0.052 ns. |
| V4B systolic-like row-pair | 19534 | HLS only | Not competitive | Not competitive | Rejected before Vivado. |
| V4C physical-control cluster | 3434 | HLS only | Same as V4A if routed | Same as V4A if routed | Rejected because HLS summary/fanout matched V4A. |

## Why This Is A Reasonable Stop Point

The final design uses all available DSPs:

```text
DSP 840 / 840
```

The cycle count is already at the best observed full-DSP schedule.  Additional
LUT/FF/BRAM headroom does not directly create more multipliers, and C++-level
attempts to force extra local copies were optimized away by HLS.  Remaining
improvement opportunities are mostly implementation-side routing variation:

```text
Vivado directive sweep
place/route seed variation
targeted RTL-level register duplication
optional floorplan constraints
```

Those are plausible for small additional gains, but the main HLS architecture
DSE has converged.
