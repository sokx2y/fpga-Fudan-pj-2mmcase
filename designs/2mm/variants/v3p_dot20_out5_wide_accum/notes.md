# v3p_dot20_out5_wide_accum

## Intent

Follow-up after v3o failed HLS throughput.

v3o tried to fix v3n's accumulation timing path with:

```cpp
#pragma HLS bind_op variable=sum op=add impl=fabric latency=2
```

That directly hit the loop-carried accumulator recurrence and raised HLS
latency to 22825 cycles.  v3p avoids latency-binding the accumulator add.

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

- product is still truncated to `DATA_TYPE` (`short`);
- dot-product accumulator is local `int`;
- final tmp/D value is cast back to `DATA_TYPE`.

## Semantic Check

The original successful variants effectively compute:

```text
acc_short = short(acc_short + short(lhs * rhs))
```

For addition, repeated 16-bit truncation is equivalent to one final 16-bit
truncation modulo 2^16:

```text
short(short(a + b) + c) == short(a + b + c)
```

The local wide accumulator does not overflow `int` here because it sums at most
100 already-truncated short products.

Final visible values remain:

```text
tmp = short(sum short products)
D   = short(sum short products)
sum = int reduction of short D
```

## Local Correctness Smoke Test

```text
expected_sum = 957248
actual_sum   = 957248
PASS
```

## Success Criteria

- HLS latency returns to the v3m/v3n range, or at least remains below v3i's
  4238 cycles
- Vivado maps DSP to 840 / 840 or less
- Vivado 4.6 ns WNS improves from v3n's -0.230 ns
- Final runtime beats v3i 4.6 ns:

```text
latency_cycles * post_route_period_ns < 19494.8 ns
```

## Reject Criteria

- HLS latency rises above v3i without a clear timing benefit
- Wider `int` accumulation creates worse carry-chain timing than v3n
- Correctness differs from expected_sum=957248

## Suggested HLS Command Context

```text
HLS_VARIANT=v3p_dot20_out5_wide_accum
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage3p_dot20_out5_wide_accum.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## Result: solution475

HLS `solution475`:

- Target clock: 4.75 ns
- Estimated clock: 3.442 ns
- Latency: 3534 cycles
- Top row loop: 99 iterations, 35 cycles/iteration
- `compute_tmp_row`: 32 cycles
- `consume_tmp_row`: 33 cycles
- DSP estimate: 1000 / 840
- BRAM_18K: 0
- FF: 36742
- LUT: 46974

Vivado project source check:

```text
E:/Vivado/fpga/v3p_dot20_out5_wide_accum/v3p_dot20_out5_wide_accum.xpr
uses:
designs/2mm/hls_proj/v3p_dot20_out5_wide_accum/solution475/syn/verilog/*.v
```

## Vivado 4.6 ns Result

Vivado implementation uses the `solution475` RTL and passes at 4.6 ns:

- Clock constraint: 4.600 ns
- WNS: +0.006 ns
- TNS: 0.000 ns
- Setup failing endpoints: 0
- WHS: +0.037 ns
- THS: 0.000 ns
- Route status: 99545 / 99545 routable nets fully routed
- Routing errors: 0
- Placed DSP: 840 / 840
- Placed LUT: 40342 / 203800
- Placed FF: 28654 / 407600
- Placed BRAM tile: 0 / 445

Conservative final runtime:

```text
runtime_ns = 3534 * 4.600 = 16256.4 ns
speedup_vs_CPU_4.632ms = 284.93x
```

Archived reports:

```text
designs/2mm/hls_proj/v3p_dot20_out5_wide_accum/vivado_impl_4p6_pass_20260611/
```

Archived implementation checkpoints:

```text
designs/2mm/hls_proj/v3p_dot20_out5_wide_accum/vivado_impl_4p6_pass_20260611/checkpoints/

kernel_2mm_opt.dcp
kernel_2mm_placed.dcp
kernel_2mm_physopt.dcp
kernel_2mm_routed.dcp
```

DRC/methodology notes:

- DRC critical warnings are `NSTD-1` and `UCIO-1`, caused by unspecified board
  I/O standard and pin locations on the bare HLS kernel ports.
- Methodology warnings are `TIMING-18`, missing input/output delay on bare
  top-level ports.
- These are expected for this kernel-only timing experiment and do not change
  the post-route clock/routing result. They must be resolved only for
  board-level bitstream generation.

Current interpretation:

v3p is the current performance champion.  The local wide accumulation preserved
the DOT20 / OUT5 throughput advantage while removing the v3n/v3o timing failure
mode enough for 4.6 ns routing closure.
