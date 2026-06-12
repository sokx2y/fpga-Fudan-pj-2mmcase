#include "kernel_2mm.h"

#define DOT_UNROLL_FACTOR 10

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE tmp[NI][NJ];
  DATA_TYPE A[NI][NK];
  DATA_TYPE B[NK][NJ];
  DATA_TYPE C[NJ][NL];
  DATA_TYPE D[NI][NL];

#pragma HLS ARRAY_PARTITION variable=A cyclic factor=10 dim=2
#pragma HLS ARRAY_PARTITION variable=B cyclic factor=10 dim=1
#pragma HLS ARRAY_PARTITION variable=tmp cyclic factor=10 dim=2
#pragma HLS ARRAY_PARTITION variable=C cyclic factor=10 dim=1

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
    for (int j = 0; j < NJ; j++) {
#pragma HLS PIPELINE II=1
      DATA_TYPE acc = SCALAR_VAL(0.0);
      for (int k = 0; k < NK; ++k) {
#pragma HLS UNROLL factor=10
        acc += A[i][k] * B[k][j];
      }
      tmp[i][j] = acc;
    }
  }

  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NL; j++) {
#pragma HLS PIPELINE II=1
      DATA_TYPE acc = D[i][j];
      for (int k = 0; k < NJ; ++k) {
#pragma HLS UNROLL factor=10
        acc += tmp[i][k] * C[k][j];
      }
      D[i][j] = acc;
    }
  }

  *sum = 0;
  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NL; j++) {
      *sum += D[i][j];
    }
  }
}
