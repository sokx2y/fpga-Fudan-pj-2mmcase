/**
 * This version is stamped on May 10, 2016
 *
 * Contact:
 *   Louis-Noel Pouchet <pouchet.ohio-state.edu>
 *   Tomofumi Yuki <tomofumi.yuki.fr>
 *
 * Web address: http://polybench.sourceforge.net
 */
/* 2mm.c: this file is part of PolyBench/C */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Include benchmark-specific header. */
#include "2mm.h"

/* Main computational kernel. The whole function will be timed,
   including the call and return. */

void kernel_2mm(DATA_TYPE seed, int *sum)
{
  DATA_TYPE i, j, k;
  DATA_TYPE tmp[NI][NJ];
  DATA_TYPE A[NI][NK];
  DATA_TYPE B[NK][NJ];
  DATA_TYPE C[NJ][NL];
  DATA_TYPE D[NI][NL];

  for(i = 0; i < NI; i++) {
    for(j = 0; j < NJ; j++) {
	tmp[i][j] = 0;
    }
  }
  for(i = 0; i < NI; i++) {
    for(j = 0; j < NK; j++) {
	A[i][j] = i + j + seed;
    }
  }
  for(i = 0; i < NK; i++) {
    for(j = 0; j < NJ; j++) {
	B[i][j] = i + j - seed;
    }
  }
  for(i = 0; i < NJ; i++) {
    for(j = 0; j < NL; j++) {
	C[i][j] = i - j + seed;
    }
  }
  for(i = 0; i < NI; i++) {
    for(j = 0; j < NL; j++) {
	D[i][j] = 0;
    }
  }

#pragma scop
/* D := A*B*C + D */
loop9:
  for (i = 0; i < NI; i++)
  loop10:
    for (j = 0; j < NJ; j++)
    {
      tmp[i][j] = SCALAR_VAL(0.0);
    loop11:
      for (k = 0; k < NK; ++k)
        tmp[i][j] += A[i][k] * B[k][j];
    }
loop12:
  for (i = 0; i < NI; i++)
  loop13:
    for (j = 0; j < NL; j++)
    {
    loop14:
      for (k = 0; k < NJ; ++k)
        D[i][j] += tmp[i][k] * C[k][j];
    }
#pragma endscop

  *sum = 0;
  for(i = 0; i < NI; i++) {
    for(j = 0; j < NL; j++) {
	*sum += D[i][j];
    }
  }
}

int main(int argc, char **argv)
{
  /* Run kernel. */
  DATA_TYPE seed;
  int sum;

  //struct timespec start, end;
  //double elapsed;
  //clock_gettime(CLOCK_MONOTONIC,&start);

  kernel_2mm(seed, &sum);

  //clock_gettime(CLOCK_MONOTONIC,&end);
  //elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

  //printf("runtime: %f s\n",elapsed);
  //printf("sum = %d\n", sum);

  /*gcc -O2 -o test 2mm.c tb.c -lrt*/
  /*.test*/
  /*runtime: 0.004632 s*/
}
