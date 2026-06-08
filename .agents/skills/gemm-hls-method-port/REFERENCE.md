# GEMM-HLS Method Port Reference

## Concrete Porting Templates

### Row-Stationary Template

- GEMM1: keep A row or A tile stationary while generated B tiles stream through.
- GEMM2: keep tmp row or tmp tile stationary while generated C tiles stream
  through.

### PE Array Scaling Template

Candidate PE shapes:

- 10x10
- 20x10
- 20x20
- 25x10
- 25x20

Every shape must handle edge tiles for 100x100 matrices.

### Process-One-Tile Template

- `process_one_tmp_tile`: compute a tile of tmp from generated A and B tiles.
- `process_one_sum_tile`: consume tmp and C tiles to update final sum or D tile.

### Tmp Tile Communication Template

- Producer: `compute_tmp_tile`.
- Channel: BRAM tile buffer or single-producer/single-consumer stream.
- Consumer: `consume_tmp_tile`.
- Sink: `reduce_sum` or optional D tile.

### Stage Decomposition Template

`generate -> compute_tmp -> consume_tmp -> reduce_sum`

Each stage must have clear ownership of buffers and communication paths.

### Wavefront Candidate

Diagonal feeding may become a wavefront PE schedule candidate. It is late-stage
only and must be validated for edge tiles and post-route timing.

### Chunked Accumulator Candidate

Use chunked partial sums to avoid one huge reduction tree. Verify that chunking
does not change required integer overflow behavior.

## Fixed Project Constraints

- Part: `xc7k325tffv900-2`.
- Toolchain: Vivado/Vitis HLS 2023.2.
- Data type: `short`.
- External result: `sum` pointer.

