#include "kernel_2mm.h"

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE tmp[NI][NJ];
  DATA_TYPE A[NI][NK];
  DATA_TYPE B[NK][NJ];
  DATA_TYPE C[NJ][NL];
  DATA_TYPE D[NI][NL];

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
      tmp[i][j] = SCALAR_VAL(0.0);
      for (int k = 0; k < NK; ++k) {
        tmp[i][j] += A[i][k] * B[k][j];
      }
    }
  }

  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NL; j++) {
      for (int k = 0; k < NJ; ++k) {
        D[i][j] += tmp[i][k] * C[k][j];
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
