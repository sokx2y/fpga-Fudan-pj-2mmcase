# Final V5B Optimization Summary

## Result

V5B is the current post-route runtime champion for the seed-based PolyBench 2mm
kernel in this repository.

```text
Variant: v5b_closed_form_row20
Source: designs/2mm/src/kernel_2mm_stage5b_closed_form_row20.cpp
Top function: kernel_2mm(short seed, int *sum)
Target part: xc7k325tffv900-2
Toolchain: Vitis HLS / Vivado 2023.2
```

## Algorithmic Change

V4A preserved the normal two-GEMM structure:

```text
tmp = A * B
D   = tmp * C
sum = sum(D)
```

V5B uses the fact that the original kernel generates `A`, `B`, and `C` from
indices and `seed`, and only exposes `sum`.  It replaces the two dot-product
GEMM stages with:

```text
closed-form tmp row recurrence
M0/M1 row moment reduction
D row recurrence
on-the-fly sum accumulation
```

The design explicitly preserves 16-bit signed `short` wrapping at tmp and D
boundaries with `ap_int<16>`.

## Correctness

The multi-seed host validation passed against the original short-semantics
golden model for:

```text
0, 1, 2, 3, 4, 7, 31, -3, 123,
32767, 32760, -32768, -32700, 30000, -30000
```

For the normal project test seed:

```text
seed = 3
expected_sum = 957248
actual_sum   = 957248
```

## HLS Result

Archived report:

```text
designs/2mm/variants/v5b_closed_form_row20/hls_4p75_20260612/kernel_2mm_csynth.rpt
```

Key HLS result:

```text
Clock target: 4.75 ns
Estimated clock: 3.393 ns
Latency: 238 cycles
Interval: 239 cycles
Top row loop: 5 row groups, 235 cycles total
HLS resources: DSP 660, FF 85989, LUT 103494, BRAM 0
```

## Vivado Result

Archived reports:

```text
designs/2mm/variants/v5b_closed_form_row20/vivado_4p50_20260612/
```

Key routed result:

```text
Constraint: 4.500 ns
Clock frequency: 222.222 MHz
Routed WNS: +0.210 ns
Routed WHS: +0.047 ns
Route status: 95766 / 95766 routable nets fully routed, 0 routing errors
Placed resources: LUT 48766 / 203800, FF 50960 / 407600, DSP 620 / 840, BRAM 0 / 445
```

Final runtime:

```text
238 cycles * 4.500 ns = 1071.0 ns
```

Speedup:

```text
vs 4.632 ms CPU baseline: 4324.93x
vs V4A 4.50 ns champion: 14.43x
```

## Comparison

| Variant | HLS cycles | Post-route clock | Final runtime | DSP | Status |
|---|---:|---:|---:|---:|---|
| V4A output-stationary cluster | 3434 | 4.500 ns | 15453.0 ns | 840 routed | Former champion |
| V5A closed-form serial rows | 4704 | pending | pending | 33 HLS | Correct but too serial |
| V5B closed-form row20 | 238 | 4.500 ns | 1071.0 ns | 620 placed | Current champion |

## Interpretation

V4A is close to the lower bound of a structure-preserving row-ping-pong 2mm
architecture.  V5B wins because it changes the dependence graph for the given
kernel and then spends FPGA resources on 20 independent row lanes.

This result should be reported as a kernel-specific algorithm/architecture
optimization for the original `seed -> sum` kernel, not as a general
external-matrix 2mm accelerator.
