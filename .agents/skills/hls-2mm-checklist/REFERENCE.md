# HLS 2mm Checklist Reference

Project target:

- Fixed Vivado part: `xc7k325tffv900-2`.
- Device family: XC7K325T, speed grade -2.

Optimization checklist:

- Is pipelining applied at a loop level that can actually improve throughput?
- Does every unroll factor have matching array partitioning or enough memory
  bandwidth?
- Are BRAM ports sufficient for the access pattern?
- Is DATAFLOW single-producer/single-consumer where required?
- Can FIFO depth cause deadlock?
- Is the reduction tree too deep for the requested clock?
- Is the critical path likely too long?
- Are DSP, BRAM, LUT, and FF usage plausible for XC7K325T?
- Does the variant record correctness, latency, clock, and resources?

Never use another package as an assumed capacity target.

