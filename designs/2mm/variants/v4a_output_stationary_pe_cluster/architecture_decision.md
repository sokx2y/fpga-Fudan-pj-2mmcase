# v4a_output_stationary_pe_cluster Architecture Decision

## Candidate

V4A is the intended architectural follow-up to the V3q champion.  It should not
be a pragma-only timing tweak.  The goal is a more regular placement-aware PE
cluster / output-stationary cluster that keeps the useful DOT20 / OUT5 work
rate while making communication more local.

Current champion boundary:

```text
v3q_dot20_out5_apint_fanout_relief
latency: 3434 cycles
post-route clock: 4.600 ns
post-route WNS: +0.006 ns
runtime: 15796.4 ns
DSP: 840 / 840
```

## Architect Six Elements

1. Hardware structure:
   Build five output-stationary clusters per active row tile.  Each cluster owns
   one output lane accumulator and a local group of DOT20 multiply lanes.  The
   cluster boundary should make A/tmp broadcast local to a cluster group rather
   than a broad cross-lane fanout across the whole DOT20 / OUT5 block.

2. Dataflow diagram:
   `row producer PE clusters -> ping/pong tmp row -> row consumer PE clusters -> lane-local sums -> final sum`

3. Storage hierarchy:
   Keep the no-BRAM V3q storage target: fully partitioned ping/pong row buffers,
   cluster-local operand registers, cluster-local dot accumulators, and
   five-lane `lane_sum`.  Do not introduce full `tmp`, full `D`, URAM, HBM, or
   board/platform assumptions.

4. GEMM1/GEMM2 communication:
   Preserve one-row ping-pong communication first.  The V4A experiment is about
   regularizing each row engine's PE placement and local routing, not yet about
   a full systolic inter-row or inter-tile stream.

5. Critical-path estimate:
   V3q's limiting paths are route dominated and near DSP inputs / local short
   arithmetic.  A cluster form should reduce high-fanout operand distribution
   and make the physical pattern more repeatable.  The main risk is that HLS
   does not preserve the intended cluster boundary and either recreates the V3q
   crossbar-like structure or adds control overhead.

6. Resource and latency model:
   The target work rate is still around 100 products per active stage across
   DOT20 / OUT5.  Resource target is the same physical envelope as V3q:

```text
DSP:        <= 840 post-synth/post-place
BRAM tile:  0 preferred
HLS cycles: target <= 3534, acceptable <= 3736
Runtime to beat: 15796.4 ns
```

## Worker Classification

Explorer / Innovator boundary.

It reuses the proven V3q algorithm and row ping-pong schedule, but changes the
compute organization into explicit PE clusters.  It is less risky than a full
systolic array and more architectural than a local pragma tune.

## T1 Hardware Checklist

| Check | Result | Note |
|---|---|---|
| Fixed part remains `xc7k325tffv900-2` | Pass | No alternate package or fallback. |
| Vitis/Vivado HLS 2023.2 compatibility | Pass | Use C++/pragmas only; no `hls::task` or `stream_of_blocks`. |
| Alveo/U50/HBM/URAM/v++ assumptions | Pass | None allowed. |
| Algorithmic semantics | Must preserve | Product truncates to `short`, tmp/D cast to `short`, final sum is int. |
| DATAFLOW topology | Pass for V4A | Keep row ping-pong; no new channel deadlock risk in first cluster step. |
| PE regularity | Required | Output-lane clusters should own local accumulators and operand registers. |
| Memory/banking pressure | Watch | Complete row partitioning remains expensive but known to route. |
| DSP pressure | Watch | Must verify Vivado maps into 840 DSP; HLS estimates may exceed device. |
| Timing goal | Required | Must improve 4.6 ns margin or pass 4.55 ns; HLS clock alone is not enough. |
| Reject if HLS-only win | Required | Final ranking still needs Vivado route/timing. |

## T1 Decision

Allowed to proceed to source design, but the implementation should be a new
cluster organization, not a latency/pragmas-only variant.

Source created:

```text
designs/2mm/src/kernel_2mm_stage4a_output_stationary_pe_cluster.cpp
```

Recommended implementation order:

1. Write a small cluster helper for one output lane with DOT20 local lanes and a
   local `ap_int<24>` accumulator.
2. Instantiate/unroll five independent output clusters for `compute_tmp_row`.
3. Mirror the same cluster structure for `consume_tmp_row`.
4. Keep row ping-pong DATAFLOW unchanged.
5. Run C-sim and HLS screening; only export to Vivado if cycles stay within the
   acceptable band.

Suggested HLS command context:

```text
HLS_VARIANT=v4a_output_stationary_pe_cluster
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage4a_output_stationary_pe_cluster.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## Not V4A

The optional `v3r_dot20_out5_dsp_input_reg` experiment is not this V4A.  It is a
local V3q timing exploiter and should not be treated as the architecture mainline.
