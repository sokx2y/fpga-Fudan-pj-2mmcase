#include "kernel_2mm.h"

// Stage 3E: DOT=20, OUT=5, full tmp, no full D, lane sums.
// Keeps DOT*OUT at 100 product lanes like v3d DOT25 OUT4, but reduces the
// number of output column groups from 25 to 20.
#define DOT_UNROLL_FACTOR 20
#define OUT_UNROLL_FACTOR 5

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

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE tmp[NI][NJ];

  #pragma HLS ARRAY_PARTITION variable=tmp cyclic factor=20 dim=2

  for (int i = 0; i < NI; i++) {
    for (int j0 = 0; j0 < NJ; j0 += OUT_UNROLL_FACTOR) {
      #pragma HLS PIPELINE II=1
      DATA_TYPE acc[OUT_UNROLL_FACTOR];
      #pragma HLS ARRAY_PARTITION variable=acc complete dim=1

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        acc[jo] = 0;
      }

      for (int k = 0; k < NK; ++k) {
        #pragma HLS UNROLL factor=20
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
          tmp[i][j] = acc[jo];
        }
      }
    }
  }

  int lane_sum[OUT_UNROLL_FACTOR];
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=1

  for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
    #pragma HLS UNROLL
    lane_sum[jo] = 0;
  }

  for (int i = 0; i < NI; i++) {
    for (int j0 = 0; j0 < NL; j0 += OUT_UNROLL_FACTOR) {
      #pragma HLS PIPELINE II=1
      DATA_TYPE acc[OUT_UNROLL_FACTOR];
      #pragma HLS ARRAY_PARTITION variable=acc complete dim=1

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        acc[jo] = 0;
      }

      for (int k = 0; k < NJ; ++k) {
        #pragma HLS UNROLL factor=20
        const DATA_TYPE tmp_val = tmp[i][k];
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

  int local_sum = 0;
  for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
    #pragma HLS UNROLL
    local_sum += lane_sum[jo];
  }

  *sum = local_sum;
}
