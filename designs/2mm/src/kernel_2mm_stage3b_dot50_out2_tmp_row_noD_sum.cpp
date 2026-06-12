#include "kernel_2mm.h"

// Stage 3B-2: DOT=50, OUT=2, tmp row buffer, no full D storage.
#define DOT_UNROLL_FACTOR 50
#define OUT_UNROLL_FACTOR 2
#define USE_TMP_ROW_BUFFER 1
#define USE_FULL_D_STORAGE 0
#define USE_ON_THE_FLY_SUM 1

#if !USE_TMP_ROW_BUFFER || USE_FULL_D_STORAGE || !USE_ON_THE_FLY_SUM
#error "stage3b_dot50_out2_tmp_row_noD_sum requires tmp row buffering and on-the-fly sum."
#endif

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE A[NI][NK];
  DATA_TYPE B[NK][NJ];
  DATA_TYPE C[NJ][NL];

  #pragma HLS ARRAY_PARTITION variable=A cyclic factor=50 dim=2
  #pragma HLS ARRAY_PARTITION variable=B cyclic factor=50 dim=1
  #pragma HLS ARRAY_PARTITION variable=C cyclic factor=50 dim=1

#if OUT_UNROLL_FACTOR > 1
  #pragma HLS ARRAY_PARTITION variable=B cyclic factor=2 dim=2
  #pragma HLS ARRAY_PARTITION variable=C cyclic factor=2 dim=2
#endif

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

  int local_sum = 0;

  for (int i = 0; i < NI; i++) {
    DATA_TYPE tmp_row[NJ];
    #pragma HLS ARRAY_PARTITION variable=tmp_row cyclic factor=50 dim=1

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
          tmp_row[j] = acc[jo];
        }
      }
    }

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
            acc[jo] += tmp_row[k] * C[k][j];
          }
        }
      }

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        const int j = j0 + jo;
        if (j < NL) {
          local_sum += acc[jo];
        }
      }
    }
  }

  *sum = local_sum;
}
