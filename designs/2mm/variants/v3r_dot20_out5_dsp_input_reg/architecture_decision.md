# v3r_dot20_out5_dsp_input_reg Architecture Decision

## Candidate

This pre-V4 timing experiment starts from the current post-route champion:

```text
v3q_dot20_out5_apint_fanout_relief
latency: 3434 cycles
post-route clock: 4.600 ns
post-route WNS: +0.006 ns
runtime: 15796.4 ns
DSP: 840 / 840
```

The candidate keeps the V3q DOT20 / OUT5 row ping-pong architecture and changes
only the DSP multiply binding from latency 2 to latency 3.  It is not the V4
placement-aware PE/cluster architecture; it is a local V3q timing exploiter.

## Architect Six Elements

1. Hardware structure:
   DOT20 / OUT5 dot-product engines for both GEMM phases, one row producer and
   one row consumer overlapped through ping-pong `tmp_row` buffers.  The design
   remains sum-only and does not materialize full `tmp` or `D`.

2. Dataflow diagram:
   `compute_tmp_row(i) -> ping/pong row buffer -> consume_tmp_row(i-1) -> lane_sum -> final sum`

3. Storage hierarchy:
   Fully partitioned 100-element ping/pong row buffers in registers, fully
   partitioned five-lane `lane_sum`, local per-output-lane operand arrays, and
   no BRAM tile requirement.

4. GEMM1/GEMM2 communication:
   GEMM1 communicates one complete `tmp` row at a time to GEMM2 through the
   existing ping-pong row buffers.  No FIFO or multi-reader DATAFLOW channel is
   introduced.

5. Critical-path estimate:
   V3q is route dominated at 4.6 ns and has DPIP-1 warnings on DSP48 A inputs.
   V4a asks HLS for a slightly deeper DSP multiply pipeline to see whether the
   DSP input-side paths gain margin.  Main risk is that the extra multiply
   latency enters the dot-product recurrence and increases row latency.

6. Resource and latency model:
   Expected DSP shape remains the V3q shape: HLS may estimate 1000 DSP, with
   Vivado mapping to 840 DSP.  Expected HLS latency is acceptable only if top
   latency stays near V3q:

```text
target:      <= 3534 cycles
acceptable: <= 3736 cycles
reject:      > 4238 cycles unless timing margin is exceptional
```

## Worker Classification

Exploiter.

This is a local timing-closure refinement of the current champion, not a new
algorithm or new storage/dataflow topology.

## T1 Hardware Checklist

| Check | Result | Note |
|---|---|---|
| Fixed part remains `xc7k325tffv900-2` | Pass | No package fallback. |
| Vitis/Vivado HLS 2023.2 compatibility | Pass | Uses existing pragmas only; no 2025.2 APIs. |
| Alveo/U50/HBM/URAM/v++ assumptions | Pass | None introduced. |
| Algorithmic semantics | Pass | Product truncates to `short`, dot result casts back to `short`, `sum` remains int. |
| DATAFLOW topology | Pass | Same row ping-pong producer/consumer structure as V3q. |
| Memory/banking pressure | Pass | Same complete partitioning; no BRAM ports added. |
| DSP pressure | Watch | Same logical 1000-DSP HLS shape as V3q; must verify Vivado still maps to 840. |
| Timing risk | Watch | Extra DSP latency may help routed paths but may hurt recurrence scheduling. |
| Deadlock risk | Pass | No new stream/FIFO/channel. |

## T1 Decision

Allowed to enter T2 HLS synthesis.

Run HLS first at 4.75 ns only if this local timing experiment is still desired:

```text
HLS_VARIANT=v3r_dot20_out5_dsp_input_reg
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage3r_dot20_out5_dsp_input_reg.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

Proceed to Vivado only if HLS latency remains at or below 3736 cycles and the
row functions remain close to V3q's 32-cycle shape.  Prefer Vivado 4.6 ns first;
try 4.55 ns only if 4.6 ns has meaningful positive margin.
