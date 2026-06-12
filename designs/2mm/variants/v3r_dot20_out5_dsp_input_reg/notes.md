# v3r_dot20_out5_dsp_input_reg

## Intent

Optional pre-V4 timing experiment from the current V3q champion.  This is not
the V4 placement-aware PE/cluster mainline; it is a local DSP input pipeline
exploiter.

V3q result:

```text
Latency: 3434 cycles
Vivado clock: 4.600 ns
Post-route WNS: +0.006 ns
Runtime: 15796.4 ns
DSP: 840 / 840
```

V3q's routed DRC reports DPIP-1 warnings on DSP48 A inputs, and the passing
4.6 ns margin is effectively zero.  V4a tests whether one more DSP multiply
latency stage can improve the routed margin or support 4.55 ns.

## Local Change From V3q

Source:

```text
designs/2mm/src/kernel_2mm_stage3r_dot20_out5_dsp_input_reg.cpp
```

Only intended source change:

```cpp
#pragma HLS bind_op variable=product op=mul impl=dsp latency=3
```

Unchanged from V3q:

- DOT20 / OUT5;
- initless A/B/C;
- row ping-pong DATAFLOW;
- no full tmp / no full D;
- `ap_int<24>` local dot accumulator;
- product truncation to `DATA_TYPE`;
- final tmp/D cast back to `DATA_TYPE`;
- per-output-lane A/tmp operand copies;
- `lane_sum` remains 32-bit `int`.

## Semantic Rationale

The visible arithmetic remains:

```text
tmp = short(sum of short products)
D   = short(sum of short products)
sum = int reduction of short D
```

Changing DSP multiply latency should not alter C behavior.  It can only change
the hardware schedule.

## Success Criteria

- C-sim PASS for `seed=3`, `expected_sum=957248`.
- HLS latency target:

```text
target:      <= 3534 cycles
acceptable: <= 3736 cycles
```

- `compute_tmp_row` and `consume_tmp_row` remain close to V3q's 32-cycle row
  latency.
- Vivado 4.6 ns WNS improves materially over V3q's +0.006 ns, or Vivado 4.55 ns
  routes with non-negative WNS.
- Vivado mapped DSP does not exceed 840 / 840.

## Reject Criteria

- HLS latency rises above 4238 cycles.
- Row function latency expands like the earlier accumulator-latency experiment.
- Vivado no longer maps the design into 840 DSP.
- 4.6 ns routed WNS is worse than V3q without a cycle reduction.

## Suggested HLS Command Context

```text
HLS_VARIANT=v3r_dot20_out5_dsp_input_reg
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage3r_dot20_out5_dsp_input_reg.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## What To Check First

After HLS:

- C simulation result;
- top latency cycles;
- `compute_tmp_row` latency;
- `consume_tmp_row` latency;
- HLS DSP/FF/LUT estimates;
- whether recurrence or II warnings appear.

If HLS remains within the acceptable latency band, export RTL and implement at
4.6 ns before attempting 4.55 ns.
