# v5a_closed_form_moment_recurrence Architecture Decision

## Candidate

V5A is an Innovator candidate for the given seed-based 2mm kernel.  It is not a
general arbitrary-matrix accelerator.  It uses the original formulas for
`A`, `B`, and `C`, plus the fact that only `sum` is observable, to replace the
two explicit GEMM dot-product phases with closed-form row generation, moment
reduction, and a `D` recurrence.

Current champion boundary:

```text
v4a_output_stationary_pe_cluster
latency: 3434 cycles
post-route clock: 4.500 ns
runtime: 15453.0 ns
DSP: 840 / 840
```

## Architect Six Elements

1. Hardware structure:
   For each row `i`, compute the two moments `M0 = sum(tmp[i][j])` and
   `M1 = sum(j * tmp[i][j])` using 20 parallel lanes.  Then generate the
   row's 100 `D` values with a 20-lane recurrence and accumulate an integer
   row sum.

2. Dataflow diagram:
   `seed/i -> closed-form tmp recurrence -> M0/M1 lanes -> D recurrence lanes -> row sums -> final sum`

3. Storage hierarchy:
   No full `A`, `B`, `C`, `tmp`, or `D` arrays.  Storage is limited to lane-local
   moment accumulators, lane-local row sums, and scalar row state.  Use
   `ap_int<16>` for explicit `short` wrapping and `ap_int<48>` for internal
   closed-form arithmetic.

4. GEMM1/GEMM2 communication:
   There is no materialized `tmp` row buffer.  GEMM1's visible row result is
   represented by the two moments needed by the sum-only GEMM2 reduction.

5. Critical-path estimate:
   Main risks are the 20-lane moment reduction tree, constant-multiply terms
   for lane offsets, and wide arithmetic in the `M1 + seed * M0` initializer.
   The design should use far fewer DSPs than V4A, so post-route timing may
   improve if HLS does not create a large control or reduction network.

6. Resource and latency model:
   With `MOMENT_UNROLL_FACTOR=20` and `OUT_UNROLL_FACTOR=20`, each row has about
   five moment iterations and five output-recurrence iterations plus reduction
   overhead.  The target HLS latency is below 2500 cycles, with an aspirational
   range near 1000-1800 cycles before Vivado routing.

## Worker Classification

Innovator.

This is an algorithm-level equivalence transform for the current kernel rather
than a local architecture tune.  It must be validated more heavily than V4A.

## T1 Hardware Checklist

| Check | Result | Note |
|---|---|---|
| Fixed part remains `xc7k325tffv900-2` | Pass | No alternate package or fallback. |
| Vitis/Vivado HLS 2023.2 compatibility | Pass | Uses fixed loops and `ap_int`; no 2025.2-only APIs. |
| Alveo/U50/HBM/URAM/v++ assumptions | Pass | None. |
| External-matrix generality | Not claimed | Candidate targets the seed-based kernel only. |
| `short` semantics | Required | Use explicit 16-bit wrapping for tmp and D boundaries. |
| Correctness validation | Required | Multi-seed C simulation before HLS result is trusted. |
| Memory/banking pressure | Pass | No matrix memories and no BRAM requirement. |
| DSP pressure | Expected low | Far below the 840-DSP V4A envelope unless HLS maps wide terms poorly. |
| Reduction timing risk | Medium | Watch moment and final lane reductions. |
| Final ranking | Required | Only post-route `latency * clock_period` can beat V4A. |

## T1 Decision

Allowed to proceed as a separate research branch.  It should not replace V4A in
the report unless it passes multi-seed correctness checks and post-route timing.

Source:

```text
designs/2mm/src/kernel_2mm_stage5a_closed_form_moment_recurrence.cpp
```

Multi-seed testbench:

```text
designs/2mm/tb/tb_kernel_2mm_multi_seed.cpp
```

Suggested HLS command context:

```text
HLS_VARIANT=v5a_closed_form_moment_recurrence
HLS_CLOCK_PERIOD_NS=4.5
HLS_SRC_FILES="../src/kernel_2mm_stage5a_closed_form_moment_recurrence.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm_multi_seed.cpp"
```

Reject if:

```text
1. Multi-seed C simulation fails.
2. HLS latency is not materially below V4A's 3434 cycles.
3. HLS creates excessive wide multipliers or a route-hostile reduction network.
4. Vivado post-route runtime does not beat 15453.0 ns.
```
