# v5b_closed_form_row20 Architecture Decision

## Candidate

V5B is the resource-spending follow-up to V5A.  V5A proved the closed-form
moment-recurrence algorithm but used only 33 DSP and serialized all 100 rows.
V5B processes 20 independent rows in parallel to trade unused resources for
latency.

Current comparison points:

```text
V4A champion: 3434 cycles at 4.500 ns post-route
V5A HLS:      4704 cycles at 4.75 ns HLS target, 33 DSP
```

## Architect Six Elements

1. Hardware structure:
   Instantiate 20 independent row lanes.  Each row lane computes one row's
   `M0/M1` moments with 20 moment lanes and then consumes the row with a
   20-lane `D` recurrence.

2. Dataflow diagram:
   `20 row ids -> 20 closed-form row engines -> 20 row sums -> final reduction`

3. Storage hierarchy:
   No full `A`, `B`, `C`, `tmp`, or `D`.  Storage is lane-local accumulators
   and final row sums only.

4. GEMM1/GEMM2 communication:
   Per row, GEMM1 is represented by `M0/M1`; GEMM2 is represented by a
   recurrence over `D[i][l]`.  Across rows, lanes are independent.

5. Critical-path estimate:
   Main risk shifts from latency to replicated wide arithmetic and final sum
   reduction.  Since each lane is locally identical to V5A, the first HLS check
   should verify that HLS actually replicated row lanes rather than sharing one
   instance.

6. Resource and latency model:
   Expected DSP usage is roughly `20 * 33 = 660 DSP`, below the 840-DSP device
   limit.  Expected HLS latency is roughly five row groups times the V5A row
   latency, plus top-level reduction overhead.  The target is far below 3434
   cycles.

## Worker Classification

Exploiter on top of the V5A Innovator route.

## T1 Hardware Checklist

| Check | Result | Note |
|---|---|---|
| Fixed part remains `xc7k325tffv900-2` | Pass | No alternate package or fallback. |
| Vitis/Vivado HLS 2023.2 compatibility | Pass | Fixed C++ templates and `ap_int`; no newer APIs. |
| Correctness risk | Medium | Same closed-form semantics as V5A; must use multi-seed testbench. |
| DSP pressure | Medium | Target around 660 DSP, below 840. |
| LUT/FF pressure | Medium | 20 replicated row engines may become route-heavy but should fit. |
| Latency goal | Required | Must beat 3434 HLS cycles materially. |
| Timing goal | Watch | Post-route clock may be worse than V4A; final metric decides. |

## Suggested HLS Command

```text
HLS_VARIANT=v5b_closed_form_row20
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage5b_closed_form_row20.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm_multi_seed.cpp"
vitis_hls -nolog -f scripts/run_hls.tcl
```

## Results

The candidate passed the intended resource-spending check.

HLS result:

```text
Latency: 238 cycles
Top loop: 5 row groups
HLS estimated resources: DSP 660, FF 85989, LUT 103494, BRAM 0
```

Vivado routed result at 4.50 ns:

```text
Routed WNS: +0.210 ns
Routed WHS: +0.047 ns
Placed resources: LUT 48766 / 203800, FF 50960 / 407600, DSP 620 / 840, BRAM 0 / 445
Final runtime: 238 * 4.500 ns = 1071.0 ns
```

Decision:

```text
Accept as the new post-route runtime champion.
```
