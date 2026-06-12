#include "kernel_2mm.h"

// Stage 3F: DOT=25, OUT=4, initless A/B/C, no full D, lane sums.
// This keeps the v3d compute shape, replaces full tmp with two row buffers, and
// overlaps GEMM1(row i) with GEMM2(row i-1) through ping-pong row ownership.
#define DOT_UNROLL_FACTOR 25
#define OUT_UNROLL_FACTOR 4

static DATA_TYPE a_value(int i, int k, short seed)
{
  return (DATA_TYPE)(i + k + seed);
}

static DATA_TYPE b_value(int k, int j, short seed)
{
  return (DATA_TYPE)(k + j - seed);
}

static DATA_TYPE c_value(int k, int j, short seed)
{
  return (DATA_TYPE)(k - j + seed);
}

static void compute_tmp_row(int i, short seed, DATA_TYPE tmp_row[NJ])
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp_row cyclic factor=25 dim=1

  for (int j0 = 0; j0 < NJ; j0 += OUT_UNROLL_FACTOR) {
    #pragma HLS PIPELINE II=1
    DATA_TYPE acc[OUT_UNROLL_FACTOR];
    #pragma HLS ARRAY_PARTITION variable=acc complete dim=1

    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      acc[jo] = 0;
    }

    for (int k = 0; k < NK; ++k) {
      #pragma HLS UNROLL factor=25
      const DATA_TYPE a_val = a_value(i, k, seed);
      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        const int j = j0 + jo;
        if (j < NJ) {
          const DATA_TYPE b_val = b_value(k, j, seed);
          acc[jo] += a_val * b_val;
        }
      }
    }

    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      const int j = j0 + jo;
      if (j < NJ) {
        tmp_row[j] = acc[jo];
      }
    }
  }
}

static void consume_tmp_row(
    int i,
    short seed,
    DATA_TYPE tmp_row[NJ],
    int lane_sum[OUT_UNROLL_FACTOR])
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp_row cyclic factor=25 dim=1
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=1

  for (int j0 = 0; j0 < NL; j0 += OUT_UNROLL_FACTOR) {
    #pragma HLS PIPELINE II=1
    DATA_TYPE acc[OUT_UNROLL_FACTOR];
    #pragma HLS ARRAY_PARTITION variable=acc complete dim=1

    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      acc[jo] = 0;
    }

    for (int k = 0; k < NJ; ++k) {
      #pragma HLS UNROLL factor=25
      const DATA_TYPE tmp_val = tmp_row[k];
      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        const int j = j0 + jo;
        if (j < NL) {
          const DATA_TYPE c_val = c_value(k, j, seed);
          acc[jo] += tmp_val * c_val;
        }
      }
    }

    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      const int j = j0 + jo;
      if (j < NL) {
        lane_sum[jo] += acc[jo];
      }
    }
  }
}

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE tmp_ping[NJ];
  DATA_TYPE tmp_pong[NJ];
  int lane_sum[OUT_UNROLL_FACTOR];

  #pragma HLS ARRAY_PARTITION variable=tmp_ping cyclic factor=25 dim=1
  #pragma HLS ARRAY_PARTITION variable=tmp_pong cyclic factor=25 dim=1
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=1

  for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
    #pragma HLS UNROLL
    lane_sum[jo] = 0;
  }

  compute_tmp_row(0, seed, tmp_ping);

  for (int i = 1; i < NI; ++i) {
    if ((i & 1) != 0) {
      {
        #pragma HLS DATAFLOW
        compute_tmp_row(i, seed, tmp_pong);
        consume_tmp_row(i - 1, seed, tmp_ping, lane_sum);
      }
    } else {
      {
        #pragma HLS DATAFLOW
        compute_tmp_row(i, seed, tmp_ping);
        consume_tmp_row(i - 1, seed, tmp_pong, lane_sum);
      }
    }
  }

  consume_tmp_row(NI - 1, seed, ((NI & 1) != 0) ? tmp_ping : tmp_pong, lane_sum);

  int local_sum = 0;
  for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
    #pragma HLS UNROLL
    local_sum += lane_sum[jo];
  }

  *sum = local_sum;
}
