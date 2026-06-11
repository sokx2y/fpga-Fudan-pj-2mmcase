# 2mm Optimization Summary Through v3q

Target:

```text
Toolchain: Vitis HLS / Vivado 2023.2
Part:      xc7k325tffv900-2
Metric:    runtime_ns = latency_cycles * post_route_clock_period_ns
Correctness: seed=3, expected_sum=957248
CPU baseline: 4.632 ms
```

## Current Champion

```text
Variant: v3q_dot20_out5_apint_fanout_relief
HLS solution used by Vivado: solution475
Latency: 3434 cycles
Vivado clock constraint: 4.600 ns
Post-route WNS: +0.006 ns
Route status: clean
Runtime: 3434 * 4.600 = 15796.4 ns
Speedup vs CPU baseline: 293.23x
DSP: 840 / 840
BRAM tile: 0
LUT: 40515
FF: 28055
```

Archived v3q reports and checkpoints:

```text
designs/2mm/hls_proj/v3q_dot20_out5_apint_fanout_relief/vivado_impl_4p6_pass_20260612/
designs/2mm/hls_proj/v3q_dot20_out5_apint_fanout_relief/vivado_impl_4p6_pass_20260612/checkpoints/
```

## Key Result Table

| Variant | Main idea | HLS cycles | Vivado result | Runtime ns | Status |
|---|---:|---:|---:|---:|---|
| v0_baseline | Direct PolyBench-style HLS baseline | 2020033 | HLS only, est 5.838 ns | screening only | Pass C-sim, slow baseline |
| v1_dotprod_unroll10 | Dot-product unroll 10 | 43021 | HLS est 7.160 ns | screening only | Pass C-sim |
| DOT50 OUT2 fullD | Strong full-storage physical baseline | 30048 | 5 ns route pass, approx 4.923 ns | approx 147926 | Stage 2.5 baseline champion |
| v3b DOT50 OUT2 noD lane_sum | Remove full D and final D reread | 20044 | 5 ns route pass, approx 4.926 ns | approx 98737 | Major cycle/resource improvement |
| v3b DOT25 OUT4 noD lane_sum | Spend more DSP with OUT4 | 18317 | 5 ns route pass, approx 4.981 ns | approx 91237 | Faster, near full DSP |
| v3d DOT25 OUT4 initless noD | Generate A/B/C on the fly | 7116 | 5 ns route pass, approx 4.963 ns | 35316.7 | Removes init/storage bottlenecks |
| v3f DOT25 OUT4 row ping-pong | DATAFLOW producer/consumer overlap | 4539 | 5 ns route pass, WNS +0.001 | 22695.0 | First successful row DATAFLOW |
| v3g DOT25 OUT4 timing relief | v3f timing cleanup | 4539 | 5 ns pass, 4.95 ns failed | approx 22700 at 5 ns | 5 ns stable, not 4.95 |
| v3i DOT25 OUT4 DSP pipeline | DSP bind latency 2, complete rows | 4238 | 4.6 ns pass, WNS +0.069 | 19494.8 | Former champion |
| v3j DOT25 OUT4 DSP MREG | More DSP pipelining | 4440 | HLS only | needs <4.39 ns to beat v3i | Reject as mainline |
| v3k DOT25 OUT4 A fanout relief | Try A operand fanout relief | 4238 | HLS same as v3i | same screening shape | Low priority |
| v3l DOT25 OUT4 static scheduler | Static pair row scheduler | 4238 | HLS same as v3i | same screening shape | No throughput gain |
| v3m DOT20 OUT5 | Reduce output tiles, fill all DSPs | 3733 | 4.6 ns route clean but WNS -0.125 | would be 17171.8 if passed | Promising but timing fail |
| v3n DOT20 OUT5 j-term relief | Localize j/seed terms | 3434 at 4.75, 3736 at 4.6 | 4.6 ns route clean but WNS -0.230 | fail | Cycles improved, timing moved |
| v3o DOT20 OUT5 add latency | Latency-bind accumulator add | 22825 | HLS only | reject | Accumulator recurrence exploded |
| v3p DOT20 OUT5 wide accum | Wide local int accumulator, final short cast | 3534 | 4.6 ns pass, WNS +0.006 | 16256.4 | Former champion |
| v3q DOT20 OUT5 ap_int/fanout relief | ap_int local accumulator, per-lane input copies | 3434 | 4.6 ns pass, WNS +0.006 | 15796.4 | Current champion |

## Optimization Path

### 1. Establish the physical boundary

The early DOT/OUT sweeps showed that HLS latency alone was misleading.  OUT4
with full `D` could reduce HLS cycles, but placement/routing reported high
congestion or failed routing.  DOT50 OUT2 fullD became the first trustworthy
post-route baseline because it routed at 5 ns.

### 2. Remove full D storage and final D reread

The first large structural gain came from computing each short `D[i][j]` and
accumulating it into lane-local sums immediately.  This preserved the original
short `D` truncation semantics while removing full `D` storage and the final
reduction pass.

### 3. Remove A/B/C initialization

Since A, B, and C are generated from loop indices and seed, storing full arrays
was unnecessary.  Generating A/B/C on the fly removed initialization cycles and
storage pressure, producing the v3d jump to 7116 cycles.

### 4. Overlap GEMM1 and GEMM2 with row ping-pong DATAFLOW

Scalar streams were not the right communication granularity because each row
needed 100 scalar transfers before useful GEMM2 work.  Row ping-pong buffers
kept the successful independent dot-product form and overlapped producer and
consumer work, reducing latency to 4539 cycles.

### 5. Use XC7K325T DSP-aware timing relief

The DOT25/OUT4 row ping-pong design still had timing pressure.  Binding the
short multiply into DSP with latency 2 and fully partitioning row buffers gave
v3i: 4238 cycles at 4.6 ns post-route.

### 6. Explore a fuller DSP shape: DOT20/OUT5

DOT20/OUT5 keeps 100 products per stage but reduces output tiles from 25 to 20.
This uses all 840 DSPs after Vivado mapping and lowers cycles, but v3m narrowly
missed 4.6 ns timing.  v3n improved HLS cycles but moved timing failures into
accumulator and DSP input paths.

### 7. Avoid multi-cycle accumulator recurrence

v3o showed that directly latency-binding `acc + product` destroys the schedule:
the row latency jumped to 226 cycles/row and total latency became 22825 cycles.

### 8. Use wide local accumulation while preserving short semantics

v3p keeps product truncation to `short`, accumulates the local dot product in
`int`, and casts the final tmp/D value back to `short`.  This preserves the
observable 16-bit modulo result for tmp and D, avoids the v3o recurrence issue,
and closes 4.6 ns timing.

### 9. Narrow local accumulation and relieve DSP input fanout

v3q keeps the v3p architecture but changes the local dot accumulator to a
conservative `ap_int<24>` and adds per-output-lane input copies for the DSP
operands.  HLS latency improves from 3534 to 3434 cycles while Vivado still
passes at 4.6 ns with WNS +0.006 ns.  This is the current best measured
runtime, but the unchanged WNS shows that the design remains at the physical
timing boundary.

## Notes For Report

- HLS DSP estimates above 840 were not always final.  v3m/v3n/v3p estimated
  1000 DSP in HLS, but Vivado mapped the implemented design to 840 / 840.
- The final ranking must use post-route timing, not HLS estimated clock.
- v3q's DRC critical warnings are bare-kernel IO warnings (`NSTD-1`, `UCIO-1`).
  They are not timing/routing failures for this kernel-only experiment.
- v3q has only +0.006 ns WNS at 4.6 ns, so 4.6 ns is close to the current
  physical boundary for this architecture.
