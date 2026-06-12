# v4c_physical_control_cluster Architecture Decision

## Architect Six Elements

1. Objective: keep the V4A 3434-cycle class while spending unused FF/LUT to
   reduce route-dominated fanout around the output-stationary clusters.
2. Architecture: V4A DOT20/OUT5 row ping-pong with cluster-local operand copies
   and five replicated tmp-row banks for GEMM2 consumption.
3. Dataflow: unchanged from V4A: compute one tmp row, ping-pong with consuming
   the previous tmp row, reduce five output lanes into `int lane_sum`.
4. Storage: no full tmp matrix and no D matrix.  The consumer temporarily holds
   five partitioned copies of one `tmp_row[100]`.
5. Correctness contract: preserve PolyBench short intermediate truncation, final
   `int` accumulation, and V4A lane-sum order.
6. Ranking metric: final `latency_cycles * post_route_clock_period_ns` on
   fixed `xc7k325tffv900-2`.

## Worker Classification

Exploiter.

This is not a new systolic route.  It exploits V4A's working schedule and uses
available non-DSP resources to try to improve physical implementation.

## T1 Hardware Checklist

```text
Fixed part: pass, xc7k325tffv900-2 only
Tool compatibility: pass, Vitis HLS 2023.2 C++/pragma subset
Memory bandwidth: pass, all hot arrays are complete-partitioned registers
DSP budget: watch, should keep V4A 840-routed class despite HLS estimate
BRAM/URAM: pass, no URAM/HBM/U50 assumptions
Latency risk: medium, tmp-row replication may add a small fixed consumer cost
Timing risk: medium, extra registers can help fanout but also add placement load
Correctness risk: low, no algebraic reassociation beyond V4A structure
```

## Expected Latency Formula

Target remains V4A class:

```text
compute_tmp_row ~= 32 cycles
consume_tmp_row ~= 32 cycles plus possible fully-unrolled tmp replication setup
top latency target <= 3534 cycles
```

## Proceed / Reject

Proceed to T2 HLS synthesis if C++ smoke test passes.

Reject if:

```text
HLS latency > 3534 cycles without a clear timing reason
HLS schedules tmp replication as a long serialized 100-cycle loop
HLS/Vivado DSP shape exceeds the device
Vivado 4.55 ns WNS is worse than V4A
short truncation semantics change
```
