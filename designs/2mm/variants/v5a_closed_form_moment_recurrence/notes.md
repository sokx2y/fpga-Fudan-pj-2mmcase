# v5a_closed_form_moment_recurrence

## Intent

V5A tests the aggressive closed-form / recurrence route discussed after the
V4A mainline result.  The goal is to beat the current V4A final runtime by
reducing the algorithmic work for the given `seed -> sum` kernel.

This is a kernel-specific algorithm transform.  It is not a general external
`A/B/C` matrix accelerator.

## Starting Point

Current champion:

```text
Variant: v4a_output_stationary_pe_cluster
Latency: 3434 cycles
Post-route clock: 4.500 ns
Runtime: 15453.0 ns
```

V5A source:

```text
designs/2mm/src/kernel_2mm_stage5a_closed_form_moment_recurrence.cpp
```

## Closed-Form Structure

For the original kernel:

```text
A[i][k] = i + k + seed
B[k][j] = k + j - seed
C[j][l] = j - l + seed
```

The first GEMM row can be generated as:

```text
tmp[i][j] =
  short(100 * (i + seed) * (j - seed)
        + 4950 * (i + j)
        + 328350)
```

For each row:

```text
M0 = sum(tmp[i][j])
M1 = sum(j * tmp[i][j])
D[i][l] = short(M1 + (seed - l) * M0)
D[i][l + 1] = short(D[i][l] - M0)
```

## Implementation Parameters

```text
MOMENT_UNROLL_FACTOR = 20
OUT_UNROLL_FACTOR    = 20
```

Expected first HLS target:

```text
latency < 2500 cycles
clock screening point: 4.5 ns
```

## Correctness Requirements

The design must preserve the original `short` behavior.  It uses `ap_int<16>`
for explicit 16-bit wrapping at the visible tmp and D boundaries, and
`ap_int<48>` for wider closed-form arithmetic.

The multi-seed testbench checks:

```text
0, 1, 2, 3, 4, 7, 31, -3, 123,
32767, 32760, -32768, -32700, 30000, -30000
```

## Suggested HLS Command

From `designs/2mm`:

```text
$env:HLS_VARIANT="v5a_closed_form_moment_recurrence"
$env:HLS_CLOCK_PERIOD_NS="4.5"
$env:HLS_SRC_FILES="../src/kernel_2mm_stage5a_closed_form_moment_recurrence.cpp ../src/kernel_2mm.h"
$env:HLS_TB_FILES="../tb/tb_kernel_2mm_multi_seed.cpp"
vitis_hls -f scripts/run_hls.tcl
```

## Status

Initial source and multi-seed C testbench created.

Local host compile/run with the Vitis HLS 2023.2 `ap_int.h` header passed:

```text
Command:
g++ -std=c++14 -I E:\Xilinx\Vitis_HLS\2023.2\include \
  designs\2mm\src\kernel_2mm_stage5a_closed_form_moment_recurrence.cpp \
  designs\2mm\tb\tb_kernel_2mm_multi_seed.cpp \
  -o .codex_tmp\v5a_multi_seed.exe

Result:
seed = 0, expected_sum = -1555520, actual_sum = -1555520
seed = 1, expected_sum = 1293888, actual_sum = 1293888
seed = 2, expected_sum = 324288, actual_sum = 324288
seed = 3, expected_sum = 957248, actual_sum = 957248
seed = 4, expected_sum = -3116608, actual_sum = -3116608
seed = 7, expected_sum = 2575680, actual_sum = 2575680
seed = 31, expected_sum = -2434752, actual_sum = -2434752
seed = -3, expected_sum = -988096, actual_sum = -988096
seed = 123, expected_sum = 2135872, actual_sum = 2135872
seed = 32767, expected_sum = 3328320, actual_sum = 3328320
seed = 32760, expected_sum = 944064, actual_sum = 944064
seed = -32768, expected_sum = -1555520, actual_sum = -1555520
seed = -32700, expected_sum = -1027648, actual_sum = -1027648
seed = 30000, expected_sum = 650176, actual_sum = 650176
seed = -30000, expected_sum = 39872, actual_sum = 39872
PASS
```

HLS C simulation and synthesis are pending.
