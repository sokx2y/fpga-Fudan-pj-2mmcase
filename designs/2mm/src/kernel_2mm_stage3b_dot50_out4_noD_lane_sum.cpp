#include "kernel_2mm.h"

// Stage 3B aggressive width probe: DOT=50, OUT=4, full tmp, no full D storage.
// This revisits OUT4 after removing full D storage and final D readback.
#define DOT_UNROLL_FACTOR 50
#define OUT_UNROLL_FACTOR 4
#define USE_FULL_D_STORAGE 0
#define USE_ON_THE_FLY_SUM 1

#if USE_FULL_D_STORAGE || !USE_ON_THE_FLY_SUM
#error "stage3b_dot50_out4_noD_lane_sum requires no full D storage and on-the-fly sum."
#endif

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE tmp[NI][NJ];
  DATA_TYPE A[NI][NK];
  DATA_TYPE B[NK][NJ];
  DATA_TYPE C[NJ][NL];

  #pragma HLS ARRAY_PARTITION variable=A cyclic factor=50 dim=2
  #pragma HLS ARRAY_PARTITION variable=B cyclic factor=50 dim=1
  #pragma HLS ARRAY_PARTITION variable=tmp cyclic factor=50 dim=2
  #pragma HLS ARRAY_PARTITION variable=C cyclic factor=50 dim=1

#if OUT_UNROLL_FACTOR > 1
  #pragma HLS ARRAY_PARTITION variable=B cyclic factor=4 dim=2
  #pragma HLS ARRAY_PARTITION variable=C cyclic factor=4 dim=2
#endif

  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NJ; j++) {
      tmp[i][j] = 0;
    }
  }

  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NK; j++) {
      A[i][j] = i + j + seed;
    }
  }

  for (int i = 0; i < NK; i++) {
    for (int j = 0; j < NJ; j++) {
      B[i][j] = i + j - seed;
    }
  }

  for (int i = 0; i < NJ; i++) {
    for (int j = 0; j < NL; j++) {
      C[i][j] = i - j + seed;
    }
  }

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
        #pragma HLS UNROLL factor=50
        for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
          #pragma HLS UNROLL
          const int j = j0 + jo;
          if (j < NJ) {
            acc[jo] += A[i][k] * B[k][j];
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
        #pragma HLS UNROLL factor=50
        for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
          #pragma HLS UNROLL
          const int j = j0 + jo;
          if (j < NL) {
            acc[jo] += tmp[i][k] * C[k][j];
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
