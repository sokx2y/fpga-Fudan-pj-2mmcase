# v4c_physical_control_cluster

## Intent

V4C starts from the V4A champion and tries to use the available LUT/FF headroom
for physical control instead of more arithmetic parallelism.  V4A already uses
all 840 DSPs, so the realistic win mode is a lower post-route clock period, not
fewer cycles.

Current champion:

```text
V4A latency: 3434 cycles
V4A routed clock: 4.550 ns
V4A WNS: +0.090 ns
V4A runtime: 15624.7 ns
V4A resources: LUT 40935, FF 30207, DSP 840, BRAM 0
```

## Source

```text
designs/2mm/src/kernel_2mm_stage4c_physical_control_cluster.cpp
```

## Structural Changes From V4A

Keep:

```text
DOT_UNROLL_FACTOR = 20
OUT_UNROLL_FACTOR = 5
row ping-pong DATAFLOW
no full tmp matrix
no D matrix
ap_int<24> local dot accumulation
short product truncation
five int lane sums
```

Change:

```text
Replicate the consumed tmp row into five complete-partitioned local banks.
Give each output cluster its own tmp bank.
Replicate row/seed operands explicitly per output cluster.
Keep this as a physical-control variant, not a systolic rewrite.
```

## Why This Uses Resources

The extra tmp banks cost roughly:

```text
5 * 100 * 16 bits = 8000 bits of local registers before HLS optimization
```

That is acceptable because V4A uses only about 7.4% of FFs and 20.1% of LUTs
post-place, while DSPs are already fully used.

## Expected Win Mode

```text
Same or near-same HLS cycles as V4A.
Better routed WNS at 4.55 ns, or pass at 4.50 / 4.45 ns.
```

This is a tail-optimization candidate.  A 1% to 3% final runtime improvement is
credible if routing improves.  A large cycle reduction is not expected because
the design is already at the full-DSP throughput boundary.

## Suggested HLS Command Context

```text
HLS_VARIANT=v4c_physical_control_cluster
HLS_CLOCK_PERIOD_NS=4.75
HLS_SRC_FILES="../src/kernel_2mm_stage4c_physical_control_cluster.cpp ../src/kernel_2mm.h"
HLS_TB_FILES="../tb/tb_kernel_2mm.cpp"
```

## Local C++ Smoke Test

Host compile/run, not HLS:

```text
g++ -std=c++11 -Idesigns/2mm/src -IE:\Xilinx\Vitis_HLS\2023.2\include designs/2mm/src/kernel_2mm_stage4c_physical_control_cluster.cpp designs/2mm/tb/tb_kernel_2mm.cpp -o .codex_tmp\stage4c_physical_control_cluster.exe
.codex_tmp\stage4c_physical_control_cluster.exe
```

Result:

```text
expected_sum = 957248
actual_sum   = 957248
PASS
```

## First HLS Things To Check

```text
1. Latency should stay close to V4A, ideally 3434 cycles.
2. If consume_tmp_row rises by about 1 cycle due fully-unrolled tmp replication,
   it can still be acceptable if Vivado timing improves.
3. If tmp replication becomes a serialized 100-cycle loop, reject this source
   shape immediately.
4. HLS resource estimates may look larger than V4A, but routed DSP must still
   fit 840/840.
```

## HLS Result And Decision

Vitis HLS 2023.2, `solution475`:

```text
Report: designs/2mm/hls_proj/v4c_physical_control_cluster/solution475/syn/report/csynth.rpt
Latency: 3434 cycles
compute_tmp_row: 32 cycles
consume_tmp_row: 32 cycles
HLS estimated resources: DSP 1000, FF 36596, LUT 47144, BRAM 0
Estimated Fmax: 290.53 MHz
```

This is identical to V4A at the HLS summary level:

```text
V4A: 3434 cycles, DSP 1000, FF 36596, LUT 47144, BRAM 0
V4C: 3434 cycles, DSP 1000, FF 36596, LUT 47144, BRAM 0
```

RTGEN also reports the same estimated max fanout as V4A:

```text
compute_tmp_row pipeline max fanout: 10464
consume_tmp_row max fanout: 10496
```

Decision:

```text
Reject current V4C source as a performance candidate.
Do not spend Vivado implementation time on this exact RTL unless only checking
tool noise.
```

Reason:

```text
The C++-level tmp-row bank replication and local operand copies were optimized
back into essentially the same HLS hardware shape as V4A.  The intended extra
FF/LUT resource usage did not survive HLS as meaningful physical structure.
```
