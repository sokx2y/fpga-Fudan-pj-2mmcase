# v4a_output_stationary_pe_cluster

## Intent

This is the V4 mainline discussed after V3q: a more regular
placement-aware PE cluster / output-stationary cluster.  The target is to keep
the 840-DSP-class work rate but reduce the route-dominated, high-fanout shape
that leaves V3q with only +0.006 ns WNS at 4.6 ns.

Do not start with a full large systolic array.  V4A should be the middle step:
regular clusters inside the proven row ping-pong schedule.

## Starting Point

Use V3q as the behavioral and performance reference:

```text
Source: designs/2mm/src/kernel_2mm_stage3q_dot20_out5_apint_fanout_relief.cpp
Latency: 3434 cycles
Post-route: 4.6 ns pass, WNS +0.006 ns
Runtime: 15796.4 ns
DSP: 840 / 840
```

V4A source:

```text
designs/2mm/src/kernel_2mm_stage4a_output_stationary_pe_cluster.cpp
```

## Intended Structure

Keep:

- DOT20 / OUT5 work shape initially;
- initless A/B/C;
- row ping-pong DATAFLOW;
- no full tmp / no full D;
- product truncation to `DATA_TYPE`;
- local `ap_int<24>` accumulation;
- final tmp/D cast back to `DATA_TYPE`;
- final `int` lane sums.

Change:

- make each output lane an explicit output-stationary cluster;
- keep DOT20 lanes local to that output cluster;
- give each cluster local operand registers and a local accumulator;
- avoid broad shared operand/control fanout where possible;
- prefer repeated regular cluster code over clever cross-lane sharing if it
  gives Vivado a more placeable structure.

## Success Criteria

- C-sim PASS for `seed=3`, `expected_sum=957248`.
- HLS latency remains competitive:

```text
target:      <= 3534 cycles
acceptable: <= 3736 cycles
```

- Vivado maps into `xc7k325tffv900-2` with no more than 840 DSP.
- Routed result beats V3q by final metric:

```text
latency_cycles * post_route_clock_period_ns < 15796.4 ns
```

- Best plausible win modes:
  - same 3434-cycle class but 4.55 ns route pass;
  - slightly more cycles but materially better clock/margin;
  - same 4.6 ns clock with useful positive WNS for future tightening.

## Suggested HLS Command Context

```text
HLS_VARIANT=v4a_output_stationary_pe_cluster
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage4a_output_stationary_pe_cluster.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## Local C++ Smoke Test

Host compile/run, not HLS:

```text
expected_sum = 957248
actual_sum   = 957248
PASS
```

## HLS Result

Vitis HLS 2023.2, `solution475`:

```text
Report: designs/2mm/hls_proj/v4a_output_stationary_pe_cluster/solution475/syn/report/csynth.rpt
Latency: 3434 cycles
HLS estimated resources: DSP 1000, FF 36596, LUT 47144, BRAM 0
```

This keeps the V3q cycle count while changing the RTL structure toward a more
regular output-stationary cluster.

## Vivado 4.55 ns Routed Checkpoint

Vivado 2023.2 implementation result from:

```text
E:\Vivado\fpga\v4a_output_stationary_pe_cluster
```

Archived under:

```text
designs/2mm/variants/v4a_output_stationary_pe_cluster/vivado_4p55_20260612/
```

Key result:

```text
Part: xc7k325tffv900-2
Constraint: 4.550 ns
Routed WNS: +0.090 ns
Route status: 101673 / 101673 routable nets fully routed, 0 routing errors
Routed/placed resources: LUT 40935 / 203800, FF 30207 / 407600, DSP 840 / 840, BRAM 0 / 445
Latency: 3434 cycles
Final runtime: 3434 * 4.550 ns = 15624.7 ns
Speedup vs 4.632 ms CPU baseline: 296.45x
```

This supersedes V3q as the current champion by final metric:

```text
V3q: 3434 * 4.600 ns = 15796.4 ns
V4A: 3434 * 4.550 ns = 15624.7 ns
Delta: -171.7 ns, about 1.09% faster
```

## Vivado 4.50 ns Final Routed Checkpoint

Vivado 2023.2 implementation result from the same project:

```text
E:\Vivado\fpga\v4a_output_stationary_pe_cluster
```

Local archive:

```text
designs/2mm/variants/v4a_output_stationary_pe_cluster/vivado_4p50_20260612/
```

Key result:

```text
Part: xc7k325tffv900-2
Constraint: 4.500 ns
Routed WNS: +0.052 ns
Routed WHS: +0.061 ns
Route status: 101509 / 101509 routable nets fully routed, 0 routing errors
Routed/placed resources: LUT 40981 / 203800, FF 30049 / 407600, DSP 840 / 840, BRAM 0 / 445
Latency: 3434 cycles
Final runtime: 3434 * 4.500 ns = 15453.0 ns
Speedup vs 4.632 ms CPU baseline: 299.75x
```

This is the final champion recorded for this DSE:

```text
V3q:      3434 * 4.600 ns = 15796.4 ns
V4A 4.55: 3434 * 4.550 ns = 15624.7 ns
V4A 4.50: 3434 * 4.500 ns = 15453.0 ns
```

## Reject Criteria

- HLS latency rises beyond 4238 cycles without a credible timing gain.
- HLS destroys the intended cluster shape or creates large mux/control networks.
- Vivado still maps to 840 DSP but WNS is worse than V3q at 4.6 ns.
- The design changes short truncation semantics.

## V4B Later

After V4A, a separate V4B can attempt a tiled systolic-like / PE-array schedule.
That should be treated as a high-risk new route with its own architecture record,
not as a quick continuation of this cluster experiment.
