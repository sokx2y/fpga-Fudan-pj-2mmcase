#include "kernel_2mm.h"

#ifndef DOT_UNROLL_FACTOR
#define DOT_UNROLL_FACTOR 10
#endif

#ifndef OUT_UNROLL_FACTOR
#define OUT_UNROLL_FACTOR 1
#endif

#ifndef USE_FULL_D_STORAGE
#define USE_FULL_D_STORAGE 1
#endif

#ifndef USE_ON_THE_FLY_SUM
#define USE_ON_THE_FLY_SUM 0
#endif

#if !USE_FULL_D_STORAGE || USE_ON_THE_FLY_SUM
#error "v2 basic boundary sweep supports only full D storage and no on-the-fly sum."
#endif

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE tmp[NI][NJ];
  DATA_TYPE A[NI][NK];
  DATA_TYPE B[NK][NJ];
  DATA_TYPE C[NJ][NL];
  DATA_TYPE D[NI][NL];

  // HLS_SWEEP_ARRAY_PARTITION_A
  // HLS_SWEEP_ARRAY_PARTITION_B_DOT
  // HLS_SWEEP_ARRAY_PARTITION_TMP
  // HLS_SWEEP_ARRAY_PARTITION_C_DOT

#if OUT_UNROLL_FACTOR > 1
  // HLS_SWEEP_ARRAY_PARTITION_B_OUT
  // HLS_SWEEP_ARRAY_PARTITION_C_OUT
  // HLS_SWEEP_ARRAY_PARTITION_D_OUT
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
    for (int j = 0; j < NL; j++) {
      D[i][j] = 0;
    }
  }

  for (int i = 0; i < NI; i++) {
    for (int j0 = 0; j0 < NJ; j0 += OUT_UNROLL_FACTOR) {
      // HLS_SWEEP_PIPELINE_GEMM1
      DATA_TYPE acc[OUT_UNROLL_FACTOR];
      // HLS_SWEEP_ACC_PARTITION_GEMM1

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        // HLS_SWEEP_UNROLL_JO_INIT_GEMM1
        acc[jo] = SCALAR_VAL(0.0);
      }

      for (int k = 0; k < NK; ++k) {
        // HLS_SWEEP_UNROLL_K_GEMM1
        for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
          // HLS_SWEEP_UNROLL_JO_MAC_GEMM1
          const int j = j0 + jo;
          if (j < NJ) {
            acc[jo] += A[i][k] * B[k][j];
          }
        }
      }

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        // HLS_SWEEP_UNROLL_JO_WRITE_GEMM1
        const int j = j0 + jo;
        if (j < NJ) {
          tmp[i][j] = acc[jo];
        }
      }
    }
  }

  for (int i = 0; i < NI; i++) {
    for (int j0 = 0; j0 < NL; j0 += OUT_UNROLL_FACTOR) {
      // HLS_SWEEP_PIPELINE_GEMM2
      DATA_TYPE acc[OUT_UNROLL_FACTOR];
      // HLS_SWEEP_ACC_PARTITION_GEMM2

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        // HLS_SWEEP_UNROLL_JO_INIT_GEMM2
        const int j = j0 + jo;
        acc[jo] = (j < NL) ? D[i][j] : SCALAR_VAL(0.0);
      }

      for (int k = 0; k < NJ; ++k) {
        // HLS_SWEEP_UNROLL_K_GEMM2
        for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
          // HLS_SWEEP_UNROLL_JO_MAC_GEMM2
          const int j = j0 + jo;
          if (j < NL) {
            acc[jo] += tmp[i][k] * C[k][j];
          }
        }
      }

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        // HLS_SWEEP_UNROLL_JO_WRITE_GEMM2
        const int j = j0 + jo;
        if (j < NL) {
          D[i][j] = acc[jo];
        }
      }
    }
  }

  *sum = 0;
  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NL; j++) {
      *sum += D[i][j];
    }
  }
}
