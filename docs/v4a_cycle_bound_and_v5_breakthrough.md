# V4A Cycle Bound And V5 Breakthrough

## Purpose

This note explains the cycle lower bound of the normal V4A-style 2mm
architecture, how the recorded `3434 cycles` result should be interpreted, and
why the V5 closed-form route can break that bound.

The discussion is for this repository's fixed kernel:

```cpp
void kernel_2mm(short seed, int *sum);
```

It is not for an arbitrary external-`A/B/C` matrix accelerator.

## V4A Is A Structure-Preserving 2mm Design

V4A still preserves the normal two-stage 2mm compute structure:

```text
tmp = A * B
D   = tmp * C
sum = sum(D)
```

It optimizes storage and scheduling:

```text
1. A/B/C are generated on demand from seed and indices.
2. Full A/B/C arrays are not stored.
3. Full D is not stored.
4. Only one tmp row is communicated at a time.
5. Two tmp row buffers are used for row-level ping-pong.
6. GEMM1 row production overlaps GEMM2 row consumption.
```

But V4A still computes every visible `tmp[i][j]` and every visible `D[i][l]`
through dot-product style arithmetic.

That point matters: V4A reduces memory and overlaps work, but it does not
remove the two GEMM dependence graph.

## V4A Work Shape

The V4A parameters are:

```text
NI = NJ = NK = NL = 100
DOT_UNROLL_FACTOR = 20
OUT_UNROLL_FACTOR = 5
```

For one `tmp` row:

```text
100 output columns / OUT5 = 20 output groups
Each output group computes 5 tmp values
Each tmp value is a length-100 dot product
DOT20 exposes 20 k-products per dot-product cluster
```

For one `D` row:

```text
100 output columns / OUT5 = 20 output groups
Each output group computes 5 D values
Each D value is a length-100 dot product over the tmp row and C
```

V4A then overlaps:

```text
produce tmp row i
consume tmp row i - 1
```

using row ping-pong DATAFLOW.

## Ideal Row-Ping-Pong Lower Bound

Let:

```text
Lp = latency to produce one tmp row
Lc = latency to consume one tmp row into D/sum
N  = number of rows = 100
```

With perfect row ping-pong overlap:

```text
total_cycles >= Lp + (N - 1) * max(Lp, Lc) + Lc
```

This is the key lower-bound formula for the normal V4A structure.

It says that after the first row is produced, each new row group can advance
only as fast as the slower of:

```text
GEMM1 row producer
GEMM2 row consumer
```

Even if memory is perfect and D storage is removed, a structure-preserving
two-GEMM row pipeline still has to pay for 100 row steps.

## V4A HLS Report Values

The V4A HLS report at:

```text
designs/2mm/hls_proj/v4a_output_stationary_pe_cluster/solution475/syn/report/kernel_2mm_csynth.rpt
```

records:

```text
compute_tmp_row latency: 32 cycles
consume_tmp_row latency: 32 cycles
top ping-pong loop: 99 iterations, 3366 cycles
top total latency: 3434 cycles
```

The top loop average is:

```text
3366 / 99 = 34 cycles per overlapped row step
```

So the implemented schedule is effectively:

```text
first tmp row prologue        about 32 cycles
99 overlapped row steps       99 * 34 = 3366 cycles
last tmp row consume epilogue about 32 cycles
small control/final reduction overhead
```

which gives the observed:

```text
3434 cycles
```

The ideal lower bound using the reported row latencies would be:

```text
32 + 99 * max(32, 32) + 32 = 3232 cycles
```

V4A is therefore only about:

```text
3434 - 3232 = 202 cycles
```

above the simple row-pipeline lower bound implied by its own `32-cycle` row
producer and consumer.

That is already very tight for this architecture class.

## Why More V4A Tuning Has Limited Room

The normal V4A structure can improve in two ways:

```text
1. Reduce row producer/consumer latency.
2. Reduce clock period after place-and-route.
```

But both are constrained.

First, V4A already uses nearly the full DSP budget after implementation:

```text
V4A post-route resources:
DSP 840 / 840
BRAM 0
```

Second, even if the row step were improved slightly, the formula still contains
100 row steps:

```text
total_cycles ~= prologue + 99 row steps + epilogue
```

For example, if the overlapped row step improved from 34 cycles to 30 cycles:

```text
32 + 99 * 30 + 32 = 3034 cycles
```

That is useful, but it is not a massive break from `3434 cycles`.

To get far below this range while keeping the same two-GEMM row-pipeline
dependence graph, the design would need either:

```text
1. much more row-level parallelism, which V4A cannot afford because DSPs are full;
2. a different dependence graph.
```

V5 chooses the second route.

## V5 Changes The Dependence Graph

V5 uses facts that are true in the original kernel:

```text
A[i][k] = i + k + seed
B[k][j] = k + j - seed
C[j][l] = j - l + seed
```

and only `sum` is externally visible.

The first GEMM can be expanded:

```text
tmp[i][j] =
  short(100 * (i + seed) * (j - seed)
        + 4950 * (i + j)
        + 328350)
```

For fixed `i`, `tmp[i][j]` is a 16-bit recurrence:

```text
tmp[i][j + 1] = short(tmp[i][j] + delta(i))
```

The second GEMM can be reduced to two row moments:

```text
M0(i) = sum_j tmp[i][j]
M1(i) = sum_j j * tmp[i][j]
```

Then:

```text
D[i][l] = short(M1(i) + (seed - l) * M0(i))
D[i][l + 1] = short(D[i][l] - M0(i))
```

So V5 no longer has to do:

```text
100 tmp dot products per row
100 D dot products per row
```

It only has to do:

```text
compute M0/M1 for the row
generate the 100 D values by recurrence
accumulate sum
```

This is why V5 can break the V4A lower bound: the V4A bound applies to a
structure that still computes both GEMM stages as dot products.  V5 is an
algorithm-level equivalence transform for the given kernel, so it has a
different cycle model.

## V5A Result Interpretation

The first V5A HLS result was:

```text
Top latency: 4704 cycles
DSP: 33
FF: 5230
LUT: 6114
BRAM: 0
```

This was not a failure of the closed-form idea.  It showed that V5A was too
serial and too resource-light.

The V5A report shows:

```text
compute_row_moments latency: 22 cycles
consume_row_recurrence latency: 19 cycles
top row loop: 100 iterations, about 47 cycles per row
```

So V5A's cycle model is approximately:

```text
100 rows * 47 cycles/row = 4700 cycles
```

The important observation is that each row is much cheaper than a normal GEMM
row, but V5A still processes rows serially.

## V5B Resource-Spending Route

V5B spends resources on row-level parallelism:

```text
MOMENT_UNROLL_FACTOR = 20
OUT_UNROLL_FACTOR    = 20
ROW_UNROLL_FACTOR    = 20
```

The 100 rows become:

```text
100 / 20 = 5 row groups
```

Using the V5A per-row estimate as a first model:

```text
ideal V5B cycles ~= 5 * 47 + group/final-reduction overhead
```

This should be far below the V4A `3434 cycles` result if HLS actually
replicates the row lanes instead of sharing one row engine.

The intended resource trade is:

```text
V5A: about 33 DSP, 4704 cycles
V5B: many replicated row engines, expected hundreds of DSP, much lower cycles
V4A: post-route 840 DSP, 3434 cycles
```

V5B is therefore the right direction after seeing V5A use only 3% of DSPs.

## Why V5 Must Be Reported Carefully

V5 is valid only for the given seed-based kernel.  It is not a general
external-matrix 2mm accelerator.

The correctness requirements are stricter than for V4A:

```text
1. Use explicit 16-bit signed wrapping for tmp and D boundaries.
2. Validate against the original kernel for multiple seed values.
3. Do not claim support for arbitrary external A/B/C matrices.
4. Rank only by latency * post-route clock period on xc7k325tffv900-2.
```

The current multi-seed host tests for V5A/V5B include:

```text
0, 1, 2, 3, 4, 7, 31, -3, 123,
32767, 32760, -32768, -32700, 30000, -30000
```

and pass against the original short-semantics golden model.

## Bottom Line

For the normal V4A 2mm architecture, a practical row-ping-pong lower-bound
model is:

```text
total_cycles >= Lp + (N - 1) * max(Lp, Lc) + Lc
```

Using V4A's reported row latencies:

```text
ideal bound ~= 32 + 99 * 32 + 32 = 3232 cycles
actual V4A = 3434 cycles
```

So V4A is already close to the lower bound of its own structure.

V5 breaks this not by a better pragma on the same GEMM structure, but by
changing the algorithmic dependence graph:

```text
two dot-product GEMM stages
  -> closed-form tmp recurrence
  -> M0/M1 row moments
  -> D recurrence
  -> sum
```

V5A proved the transform but did not spend resources.  V5B is the intended
resource-spending version: process 20 rows in parallel and use the unused
XC7K325T resources to move below the V4A cycle region.
