# 2mm Kernel Optimization Boundary

## Purpose

This note clarifies why the current V4A implementation and a possible
closed-form recurrence implementation are valid for this repository's 2mm task,
and how they differ from a fully general external-matrix 2mm accelerator.

The project target is the original kernel behavior in `src/2mm.c` and
`designs/2mm/src/kernel_2mm.h`:

```cpp
void kernel_2mm(short seed, int *sum);
```

The observable interface is `seed -> sum`. The matrices `A`, `B`, and `C` are
not external DRAM inputs in this kernel. They are generated inside the kernel
from loop indices and `seed`.

## Original Kernel Semantics

The original 2mm code constructs:

```text
A[i][k] = i + k + seed
B[k][j] = k + j - seed
C[j][l] = j - l + seed
```

Then it computes:

```text
tmp = A * B
D   = tmp * C
sum = sum of all D elements
```

All matrix elements are `short`, and the final externally visible result is the
integer `sum`.

Therefore, a hardware design is functionally correct for this task if, for the
same `seed`, it produces the same `sum` as the original kernel while preserving
the relevant `short` truncation/wrap behavior.

## What A General External-Matrix 2mm Accelerator Means

A fully general 2mm accelerator would have an interface like:

```text
input A[100][100]
input B[100][100]
input C[100][100]
output D or sum(D)
```

Such a design must support arbitrary matrix values supplied from outside the
kernel. It may generate addresses, tile coordinates, loop counters, or stream
control on the fly, but it must read the actual values of `A`, `B`, and `C`
from its input interface or memory system.

That is a different problem from this repository's current kernel, whose only
input is `seed`.

## Why V4A Is Not A General External-Matrix Accelerator

V4A keeps the original kernel interface:

```cpp
void kernel_2mm(short seed, int *sum);
```

It does not have external ports for arbitrary `A`, `B`, and `C` matrices.
Instead, it generates the operand values when they are needed:

```text
a = i + k + seed
b = k + j - seed
c = k - l + seed
```

This means V4A cannot process arbitrary external matrices. If a user supplies a
different matrix value, there is no interface through which V4A can receive it.

However, this does not make V4A invalid for the current task. It is valid
because the original kernel itself defines `A`, `B`, and `C` using exactly these
formulas. V4A removes unnecessary matrix storage and generates operands
on-the-fly while still preserving the two-stage 2mm computation structure:

```text
generate tmp row as A * B
consume tmp row as tmp * C
accumulate sum
```

This is best described as:

```text
an optimized FPGA implementation of the given seed-based 2mm kernel
```

not as:

```text
a general-purpose arbitrary-matrix 2mm accelerator
```

## Why On-The-Fly Operand Generation Is Legitimate Here

On-the-fly generation has two different meanings:

1. A general accelerator may generate addresses or control signals on the fly
   while reading matrix values from external memory.
2. This 2mm kernel can generate the actual matrix element values on the fly
   because the original source code defines those values by formulas.

V4A uses the second kind. It is legitimate because it is equivalent to the
original kernel's matrix initialization loops, but it avoids storing full
`A`, `B`, and `C` arrays.

This is a normal hardware optimization:

```text
remove redundant storage
reduce local memory pressure
avoid unnecessary initialization loops
generate deterministic operands near the compute lanes
```

## Closed-Form Recurrence As A More Aggressive Variant

A future closed-form or recurrence variant would go one step further. Instead
of only removing `A`, `B`, and `C` storage, it would use the formulas for
`A`, `B`, and `C` plus the fact that only `sum` is observable to reduce the
amount of arithmetic.

For example, the first matrix multiplication can be expanded:

```text
tmp[i][j] =
  short(100 * (i + seed) * (j - seed)
        + 4950 * (i + j)
        + 328350)
```

For each fixed `i`, `tmp[i][j]` is a 16-bit recurrence:

```text
tmp[i][j + 1] = short(tmp[i][j] + delta(i))
```

The second multiplication can be reduced using per-row moments:

```text
M0 = sum(tmp[i][j])
M1 = sum(j * tmp[i][j])
D[i][l] = short(M1 + (seed - l) * M0)
D[i][l + 1] = short(D[i][l] - M0)
```

This candidate is more aggressive than V4A because it no longer executes the
two GEMM stages in their original dot-product form. It should therefore be
reported as an algorithm-level equivalence transform for the given kernel, not
as a general matrix accelerator.

## Fit To The Project Requirement

The stated project requirement is to optimize the algorithm and circuit
architecture for the original 2mm algorithm on the XC7K325T device, keep the
design within chip resources, verify correctness, and evaluate:

```text
runtime = latency * post_route_clock_period
```

Under that requirement, V4A fits because:

```text
1. It implements the original seed-based kernel interface.
2. It preserves the original A/B/C generation semantics.
3. It preserves the two-stage 2mm compute structure.
4. It avoids unnecessary A/B/C/D storage and uses row-level buffering.
5. It has C simulation, HLS, and Vivado post-route evidence.
6. Its final metric is computed from latency and routed clock period.
```

A closed-form recurrence variant can also fit, provided that:

```text
1. It is validated against the original kernel for multiple seed values.
2. It explicitly preserves 16-bit signed `short` behavior.
3. It is not described as a general arbitrary-matrix accelerator.
4. It passes simulation and, if shortlisted, RTL/Vivado implementation.
5. It is ranked only by latency * post-route clock period on xc7k325tffv900-2.
```

## Recommended Report Wording

Use wording like:

```text
This design targets the given PolyBench 2mm kernel, whose observable interface
is seed -> sum. In the original source, A, B, and C are generated internally
from loop indices and seed rather than supplied as arbitrary external matrices.
Therefore, the hardware eliminates full A/B/C storage and generates operands
on demand while preserving the original short-precision 2mm semantics.
```

For V4A:

```text
V4A is a structure-preserving implementation: it still computes tmp = A * B
and D = tmp * C in row-ping-pong form, but it generates deterministic operands
on the fly and accumulates the final sum without storing full D.
```

For a closed-form recurrence variant:

```text
The recurrence variant is a kernel-specific algorithm transform. It uses the
known A/B/C formulas and the sum-only output to replace the two dot-product
GEMM stages with bit-accurate closed-form and recurrence computations. It is
valid for this seed-based kernel after equivalence validation, but it is not a
general external-matrix 2mm accelerator.
```

## Bottom Line

V4A is not a general arbitrary-`A/B/C` 2mm accelerator. It is an optimized FPGA
implementation of the given `seed -> sum` 2mm kernel.

That matches the current repository and project requirement as long as the task
is to implement and optimize the original source kernel, rather than to build a
library-style accelerator for arbitrary external matrices.
