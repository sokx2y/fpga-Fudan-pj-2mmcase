#include <cstdio>

#include "../src/kernel_2mm.h"

static short to_short_wrap(int value)
{
  const unsigned int bits = (unsigned int)value & 0xffffu;
  return (bits >= 0x8000u) ? (short)(int)(bits - 0x10000u) : (short)(int)bits;
}

static int golden_kernel_2mm(short seed)
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
      A[i][j] = to_short_wrap(i + j + seed);
    }
  }

  for (int i = 0; i < NK; i++) {
    for (int j = 0; j < NJ; j++) {
      B[i][j] = to_short_wrap(i + j - seed);
    }
  }

  for (int i = 0; i < NJ; i++) {
    for (int j = 0; j < NL; j++) {
      C[i][j] = to_short_wrap(i - j + seed);
    }
  }

  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NL; j++) {
      D[i][j] = 0;
    }
  }

  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NJ; j++) {
      int acc = 0;
      for (int k = 0; k < NK; ++k) {
        acc += (int)A[i][k] * (int)B[k][j];
        acc = (int)to_short_wrap(acc);
      }
      tmp[i][j] = (DATA_TYPE)acc;
    }
  }

  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NL; j++) {
      int acc = 0;
      for (int k = 0; k < NJ; ++k) {
        acc += (int)tmp[i][k] * (int)C[k][j];
        acc = (int)to_short_wrap(acc);
      }
      D[i][j] = (DATA_TYPE)acc;
    }
  }

  int sum = 0;
  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NL; j++) {
      sum += D[i][j];
    }
  }

  return sum;
}

int main()
{
  const short seeds[] = {
      0, 1, 2, 3, 4, 7, 31, -3, 123,
      32767, 32760, -32768, -32700, 30000, -30000};

  for (unsigned int idx = 0; idx < sizeof(seeds) / sizeof(seeds[0]); ++idx) {
    const short seed = seeds[idx];
    const int expected_sum = golden_kernel_2mm(seed);
    int actual_sum = 0;

    kernel_2mm(seed, &actual_sum);

    std::printf("seed = %d, expected_sum = %d, actual_sum = %d\n",
                (int)seed, expected_sum, actual_sum);

    if (actual_sum != expected_sum) {
      std::printf("FAIL\n");
      return 1;
    }
  }

  std::printf("PASS\n");
  return 0;
}
