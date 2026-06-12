# 2mm Architecture Catalog

Target: Vivado/Vitis HLS 2023.2, fixed part `xc7k325tffv900-2`, PolyBench 2mm
with NI = NJ = NK = NL = 100, `DATA_TYPE = short`, and top function
`kernel_2mm(short seed, int *sum)`.

The final metric is:

`runtime_ns = latency_cycles * post_route_clock_period_ns`

HLS latency models in this catalog are screening estimates. They can identify
promising candidates, but final ranking requires RTL validation and Vivado
post-route timing on `xc7k325tffv900-2`.

## Architecture 1: baseline_two_gemm_bram_tmp

### Hardware Structure

Two sequential GEMM phases with full `tmp[100][100]` stored in local memory.
Arrays A, B, C, D, and tmp follow the original algorithm shape.

### Dataflow Diagram

`generate A/B/C/D -> compute tmp = A*B -> compute D = tmp*C -> reduce D to sum`

### GEMM1/GEMM2 Communication

GEMM1 writes the complete tmp matrix. GEMM2 reads tmp after GEMM1 completes.

### Storage Hierarchy

Local arrays inferred as BRAM/register memories depending on partitioning.
No streaming between GEMM phases.

### Critical Path Analysis

Main risk is multiply-accumulate path in inner loops and final sum reduction.
Without optimization, critical path should be simple but latency is high.

### Resource Scaling Model

Storage scales with full A, B, C, D, and tmp matrices. Compute resources scale
with explicit unroll factors, initially near one MAC lane.

### Expected Latency Model

Approximate latency is dominated by two 100^3 nested GEMM loops plus matrix
initialization and final 100^2 reduction.

### Applicable Conditions

Use as correctness-first reference once baseline implementation is requested.

### Non-Applicable Conditions

Do not use as final performance design if tmp storage and sequential phases
dominate runtime.

### HLS Implementation Notes

Keep semantics close to original source. Avoid aggressive pragmas until C
simulation is stable.

### Correctness Risks

Low if `short` intermediate behavior and `sum` accumulation match the original.

## Architecture 2: unrolled_dot_product_engine

### Hardware Structure

Small parallel MAC engine for inner k loops, using unroll factors such as 5, 10,
20, or 25.

### Dataflow Diagram

`generate tile operands -> parallel dot-product lanes -> local reduction -> tmp/D`

### GEMM1/GEMM2 Communication

Can still use full tmp memory. This architecture mainly changes compute width.

### Storage Hierarchy

Array partitioning is required on the dimension consumed by unrolled lanes.

### Critical Path Analysis

Risk shifts to reduction tree depth and BRAM read fanout.

### Resource Scaling Model

DSP use scales with unroll factor. BRAM ports and partitioned memory banks scale
with operand bandwidth.

### Expected Latency Model

For an unroll factor U, inner-loop compute cycles ideally approach
`ceil(100 / U)` per dot product plus reduction and pipeline overhead.

### Applicable Conditions

Try after a correct baseline when reports show inner dot products dominate.

### Non-Applicable Conditions

Reject when memory banking cannot feed the lanes or WNS collapses.

### HLS Implementation Notes

Pair every unroll factor with an explicit partition/banking plan.

### Correctness Risks

Changing reduction grouping can alter overflow behavior. Verify against the
original `short` semantics.

## Architecture 3: sum_only_no_D_store

### Hardware Structure

Compute final contributions and accumulate `sum` without materializing full D.

### Dataflow Diagram

`tmp production -> D contribution calculation -> direct sum reduction`

### GEMM1/GEMM2 Communication

May use full tmp or tile tmp, but D is replaced by partial sum accumulation.

### Storage Hierarchy

Removes D storage. Adds partial sum registers or small reduction buffers.

### Critical Path Analysis

The final reduction can become critical if too many products feed one sum tree.

### Resource Scaling Model

Saves D memory. Uses adders/registers for partial sums.

### Expected Latency Model

Eliminates D initialization and D readback reduction loops, but GEMM2 compute
still dominates unless combined with tiling or column-sum transforms.

### Applicable Conditions

Try after equivalence tests prove D storage is not externally observable.

### Non-Applicable Conditions

Reject if it changes intermediate truncation, overflow, or reduction order
required by the reference behavior.

### HLS Implementation Notes

Build a golden model specifically for sum-only equivalence before synthesis.

### Correctness Risks

High: C_colsum and sum-only transforms can change `short` intermediate
truncation semantics. Equivalence must be proven first.

## Architecture 4: c_colsum_reduced_second_gemm

### Hardware Structure

Precompute column sums of C to reduce the second GEMM into weighted tmp row
accumulation for final sum.

### Dataflow Diagram

`generate C -> colsum(C)`, `compute tmp -> accumulate tmp * C_colsum`

### GEMM1/GEMM2 Communication

GEMM2 no longer consumes each C element individually for every D cell; it
consumes C column sums for final sum.

### Storage Hierarchy

Stores C column sums and tmp rows or tiles. May remove D storage.

### Critical Path Analysis

Critical path may move to final weighted reduction.

### Resource Scaling Model

Saves second GEMM work but adds colsum storage and reduction lanes.

### Expected Latency Model

The second 100^3 phase can reduce toward 100^2 multiply/add work plus C colsum
setup, if equivalence holds.

### Applicable Conditions

Try only as an Explorer candidate with strong correctness checks.

### Non-Applicable Conditions

Reject if original `short` D element truncation affects final sum.

### HLS Implementation Notes

Keep a mode that can compare original D-based sum with C-colsum sum.

### Correctness Risks

Very high: algebraic reassociation may not preserve fixed-width C semantics.

## Architecture 5: tmp_tile_buffer_fused_gemm

### Hardware Structure

GEMM1 produces tmp tiles that GEMM2 consumes before the complete tmp matrix is
materialized.

### Dataflow Diagram

`generate A/B tile -> compute tmp tile -> buffer/stream tmp tile -> consume C tile -> partial sum/D`

### GEMM1/GEMM2 Communication

This is the key two-GEMM communication design. Tmp is communicated through a
tile buffer or FIFO-like local channel.

### Storage Hierarchy

Tile buffers for A, B, C, and tmp. Optional D tile or sum-only accumulator.

### Critical Path Analysis

Risk includes producer/consumer imbalance, FIFO pressure, and tile-local
reduction fan-in.

### Resource Scaling Model

BRAM scales with tile sizes. DSP scales with compute lanes. FIFO/register
resources scale with tmp tile communication width.

### Expected Latency Model

Latency approaches initialization plus overlapped tile production/consumption,
bounded by the slower of GEMM1 tile compute and GEMM2 tile consume.

### Applicable Conditions

Try after baseline and unroll reports identify tmp memory traffic as a bottleneck.

### Non-Applicable Conditions

Reject if dataflow deadlocks or if tmp tile reuse cannot be scheduled cleanly.

### HLS Implementation Notes

Use explicit producer/consumer ownership. Avoid multi-writer/multi-reader
DATAFLOW channels.

### Correctness Risks

Medium to high: fused scheduling must preserve all tmp values needed by GEMM2.

## Architecture 6: row_stationary_tile_pipeline

### Hardware Structure

Keep A rows or tmp rows stationary while streaming compatible B/C tiles through
the compute lanes.

### Dataflow Diagram

`load/generate stationary row tile -> sweep counterpart tiles -> accumulate row/tile result`

### GEMM1/GEMM2 Communication

GEMM1 can produce row-wise tmp tiles; GEMM2 can consume tmp rows against C tiles.

### Storage Hierarchy

Stationary row/tile registers plus BRAM tile buffers for the moving operand.

### Critical Path Analysis

MAC lane and local reduction are key. Scheduler complexity can increase II.

### Resource Scaling Model

Register pressure scales with stationary tile width. BRAM bandwidth scales with
moving operand tile stream.

### Expected Latency Model

Latency depends on tile count times per-tile dot-product cycles, with potential
overlap between load/generate and compute stages.

### Applicable Conditions

Use when row reuse is clear and tile boundaries handle 100 cleanly.

### Non-Applicable Conditions

Reject if stationary tile storage creates too much register or routing pressure.

### HLS Implementation Notes

Make edge tiles explicit. Avoid assuming dimensions are multiples of tile size.

### Correctness Risks

Medium: edge-tile handling and partial accumulation order are common failure
points.

## Architecture 7: scaled_systolic_pe_array

### Hardware Structure

Parameterized PE array candidate, such as 10x10, 20x10, 20x20, 25x10, or
25x20, adapted from GEMM-HLS-style systolic thinking.

### Dataflow Diagram

`generate/feed operands -> wavefront PE array -> chunked accumulators -> tmp tile or sum tile`

### GEMM1/GEMM2 Communication

Can produce tmp tiles into a buffer for GEMM2 or fuse with downstream consumers
in a later-stage design.

### Storage Hierarchy

PE-local registers, operand tile buffers, partial sum buffers, and optional tmp
tile buffers.

### Critical Path Analysis

Risk is high: PE routing, wavefront control, accumulator depth, and WNS can
dominate on XC7K325T.

### Resource Scaling Model

DSP use scales with PE count. Registers and LUTs scale with PE state and
interconnect. BRAM scales with operand and tmp tile buffers.

### Expected Latency Model

Ideal tile latency depends on wavefront fill/drain plus K dimension chunks, but
real latency must include control, edge tiles, and post-route clock loss.

### Applicable Conditions

Only a late-stage candidate after smaller architectures establish bottlenecks
and resource headroom.

### Non-Applicable Conditions

Do not use as baseline. Reject if it assumes 32x32 FP32, 300 MHz, U50, URAM,
512-bit AXI, or dimensions divisible by 32.

### HLS Implementation Notes

Parameterize PE shape and edge behavior. Keep a scalar or tiled golden path for
equivalence.

### Correctness Risks

High: wavefront indexing, edge tiles, accumulator chunking, and overflow order
can all break equivalence.
