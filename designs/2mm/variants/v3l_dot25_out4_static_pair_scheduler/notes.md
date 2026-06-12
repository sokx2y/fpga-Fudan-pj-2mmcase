# v3l_dot25_out4_static_pair_scheduler

## Intent

Control/scheduler-overhead experiment based on the v3i champion.

This is not a larger parallel architecture. It keeps:

- DOT25 / OUT4
- initless A/B/C
- row ping-pong DATAFLOW
- no full tmp
- no full D
- lane_sum reduction
- complete row-buffer partitioning
- `bind_op` DSP multiply with latency 2

## Local Change From v3i

v3i schedules each row with a runtime `if ((i & 1) != 0)` branch that selects
which buffer is produced and which buffer is consumed.

v3l changes only the top scheduler:

```text
compute row 0 into ping

for i = 1, 3, 5, ..., 97:
  DATAFLOW: compute row i     into pong, consume ping
  DATAFLOW: compute row i + 1 into ping, consume pong

DATAFLOW: compute row 99 into pong, consume ping
consume pong
```

It also removes the unused row-index argument from `consume_tmp_row`.

## Hypothesis

v3i at 4.6 ns is already close to the arithmetic/datapath limit:

- HLS latency: 4238 cycles
- Row loop iteration latency: about 42 cycles
- Routed worst path is route/control dominated, not arithmetic dominated

The goal is to see whether a static two-row ping/pong schedule can reduce
top-level control muxing, clock-enable fanout, or HLS row scheduling overhead.

## Success Criteria

- C-sim PASS, seed=3, expected_sum=957248
- HLS latency less than or equal to 4238 cycles, or only negligibly worse with
  clearly better Vivado timing
- DSP remains 800
- `compute_tmp_row` remains near 36 cycles
- `consume_tmp_row` remains near 40 cycles
- Vivado at 4.6 ns is at least as clean as v3i, or 4.55 ns improves over v3i

## Reject Criteria

- HLS latency rises materially above 4238 cycles
- DSP drops below 800, indicating lost parallelism
- The top loop interval increases
- Vivado timing/route is worse than v3i at the same clock

## Suggested HLS Command Context

```text
HLS_VARIANT=v3l_dot25_out4_static_pair_scheduler
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage3l_dot25_out4_static_pair_scheduler.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## How To Judge

First compare HLS against v3i:

```text
v3i: latency 4238, estimated 3.442 ns, compute 36, consume 40, DSP 800
```

If v3l has the same HLS latency and resources, only run Vivado at a tighter
clock such as 4.55 ns. A 4.6 ns rerun mainly measures implementation
randomness, because v3i already passes 4.6 ns.

## Local Correctness Smoke Test

Host C++ compile/run with the project testbench:

```text
expected_sum = 957248
actual_sum   = 957248
PASS
```

## Result: solution475

HLS `solution475`:

- Target clock: 4.75 ns
- Estimated clock: 3.442 ns
- Latency: 4238 cycles
- `compute_tmp_row`: 36 cycles
- `consume_tmp_row`: 40 cycles
- Top loop: 49 iterations, 84 cycles/iteration
- DSP: 800 / 840
- BRAM_18K: 0 / 890
- FF: 43309 / 407600
- LUT: 49327 / 203800

Interpretation:

The static two-row scheduler did not reduce total HLS cycles.  It changed the
top-level accounting from v3i's `99 * 42` row loop shape into `49 * 84`, which
is essentially the same throughput.  It also slightly increases top-level
FF/LUT versus v3i (`43309/49327` vs `43228/49237` in HLS estimates).

Recommendation:

Do not spend a normal 4.6 ns Vivado implementation run on v3l.  If it is tested
at all, use it only as a low-priority 4.55 ns physical-control experiment after
v3i's tighter clock boundary is known.
