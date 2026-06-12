# v3o_dot20_out5_add_latency_relief

## Intent

Targeted follow-up to v3n.

Keep the high-performance DOT20 / OUT5 row-pingpong structure, but target the
new v3n 4.6 ns worst path:

```text
add_ln29_* register -> CARRY4 chain -> add_ln29_* register
```

v3n proved that j-term localization can reduce HLS cycles/resource pressure,
but Vivado timing moved into the short accumulation path.  v3o therefore asks
HLS to schedule the short `acc + product` add with explicit fabric latency.

## Local Change From v3n

Keep:

- DOT20 / OUT5
- initless A/B/C
- row ping-pong DATAFLOW
- no full tmp
- no full D
- lane_sum reduction
- complete row-buffer partitioning
- per-output-lane `j - seed` and `seed - j` terms
- `bind_op` DSP multiply with latency 2

Change:

```cpp
DATA_TYPE sum = (DATA_TYPE)(acc + product);
#pragma HLS bind_op variable=sum op=add impl=fabric latency=2
```

## Why This Should Help

v3n 4.6 ns failed with:

```text
WNS: -0.230 ns
TNS: -229.771 ns
Setup failing endpoints: 3392

Worst path:
  add_ln29_* register -> add_ln29_* register
  Data Path Delay: 4.380 ns = logic 0.876 ns + route 3.504 ns
  Logic Levels: 5 (CARRY4=4 LUT3=1)
```

The goal is not to reduce DSP use or lower parallelism.  The goal is to keep
DOT20 / OUT5 and spend a little schedule slack/registering to shorten the
routed accumulation path.

## Success Criteria

- C-sim PASS, seed=3, expected_sum=957248
- HLS latency remains below v3i's 4238 cycles, ideally close to v3n/v3m
- Vivado maps DSP to 840 / 840 or less
- Vivado 4.6 ns WNS improves materially from v3n's -0.230 ns
- Final runtime beats v3i 4.6 ns:

```text
latency_cycles * post_route_period_ns < 19494.8 ns
```

## Reject Criteria

- HLS does not accept the add `bind_op` syntax
- HLS latency rises above v3i without enough timing gain
- Worst path remains the same accumulation carry path with no WNS improvement

## Local Correctness Smoke Test

```text
expected_sum = 957248
actual_sum   = 957248
PASS
```

## Suggested HLS Command Context

```text
HLS_VARIANT=v3o_dot20_out5_add_latency_relief
HLS_CLOCK_PERIOD_NS=4.60
HLS_SRC_FILES="../src/kernel_2mm_stage3o_dot20_out5_add_latency_relief.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## Result: solution475

HLS `solution475`:

- Latency: 22825 cycles
- Top row loop: 99 iterations, 226 cycles/iteration
- `compute_tmp_row`: 224 cycles
- `consume_tmp_row`: 223 cycles
- DSP estimate: 1000 / 840
- BRAM_18K: 0
- FF: 17730
- LUT: 53238

Reject.

The explicit `latency=2` fabric add touches the accumulator recurrence.  HLS
keeps the arithmetic valid by stretching the schedule, which destroys the
DOT20 / OUT5 row throughput.  Do not run Vivado for this variant.

Follow-up: avoid latency-binding the accumulator add.  If targeting the same
timing path, use an algebraically equivalent local wide accumulation or
explicit reduction structure that does not introduce a multi-cycle
loop-carried dependency.
