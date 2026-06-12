#include "kernel_2mm.h"
#include <ap_int.h>

// Stage 5A: closed-form / moment-recurrence implementation for the given
// seed-based 2mm kernel.
//
// This is not a general external-matrix 2mm accelerator.  It preserves the
// observable behavior of kernel_2mm(seed, sum) by using the original formulas
// for A/B/C and explicit 16-bit wrapping at tmp and D boundaries.
#define MOMENT_UNROLL_FACTOR 20
#define OUT_UNROLL_FACTOR 20

typedef ap_int<16> s16_t;
typedef ap_int<18> idx_t;
typedef ap_int<24> delta_t;
typedef ap_int<32> m0_t;
typedef ap_int<40> formula_t;
typedef ap_int<40> m1_t;
typedef ap_int<48> wide_t;

static s16_t narrow16(wide_t value)
{
  #pragma HLS INLINE
  return (s16_t)value;
}

static void compute_row_moments(int i, short seed, m0_t *m0, m1_t *m1)
{
  #pragma HLS INLINE off

  m0_t m0_lane[MOMENT_UNROLL_FACTOR];
  m1_t m1_lane[MOMENT_UNROLL_FACTOR];
  #pragma HLS ARRAY_PARTITION variable=m0_lane complete dim=1
  #pragma HLS ARRAY_PARTITION variable=m1_lane complete dim=1

  for (int lane = 0; lane < MOMENT_UNROLL_FACTOR; ++lane) {
    #pragma HLS UNROLL
    m0_lane[lane] = 0;
    m1_lane[lane] = 0;
  }

  const idx_t seed_w = (idx_t)seed;
  const idx_t i_w = (idx_t)i;
  const idx_t i_plus_seed = i_w + seed_w;
  const ap_int<8> c_neg100 = -100;
  const ap_int<8> c_100 = 100;
  const ap_int<14> c_4950 = 4950;
  const ap_int<20> c_328350 = 328350;
  const ap_int<36> seed_product = seed_w * i_plus_seed;
  const formula_t t0 =
      (formula_t)(c_neg100 * seed_product) +
      (formula_t)(c_4950 * i_w) +
      (formula_t)c_328350;
  const delta_t delta = (delta_t)(c_100 * i_plus_seed) + (delta_t)c_4950;

  formula_t row_base = t0;

  for (int j0 = 0; j0 < NJ; j0 += MOMENT_UNROLL_FACTOR) {
    #pragma HLS PIPELINE II=1
    for (int lane = 0; lane < MOMENT_UNROLL_FACTOR; ++lane) {
      #pragma HLS UNROLL
      const int j = j0 + lane;
      const ap_int<6> lane_idx = (ap_int<6>)lane;
      const s16_t tmp = narrow16((wide_t)(row_base + lane_idx * delta));
      m0_lane[lane] += (m0_t)tmp;
      m1_lane[lane] += (m1_t)((ap_int<8>)j * tmp);
    }

    row_base += (ap_int<6>)MOMENT_UNROLL_FACTOR * delta;
  }

  m0_t local_m0 = 0;
  m1_t local_m1 = 0;

  for (int lane = 0; lane < MOMENT_UNROLL_FACTOR; ++lane) {
    #pragma HLS UNROLL
    local_m0 += m0_lane[lane];
    local_m1 += m1_lane[lane];
  }

  *m0 = local_m0;
  *m1 = local_m1;
}

static int consume_row_recurrence(short seed, m0_t m0, m1_t m1)
{
  #pragma HLS INLINE off

  int lane_sum[OUT_UNROLL_FACTOR];
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=1

  for (int lane = 0; lane < OUT_UNROLL_FACTOR; ++lane) {
    #pragma HLS UNROLL
    lane_sum[lane] = 0;
  }

  const s16_t seed_w = (s16_t)seed;
  wide_t d_base = (wide_t)m1 + (wide_t)(seed_w * m0);

  for (int l0 = 0; l0 < NL; l0 += OUT_UNROLL_FACTOR) {
    #pragma HLS PIPELINE II=1
    for (int lane = 0; lane < OUT_UNROLL_FACTOR; ++lane) {
      #pragma HLS UNROLL
      const ap_int<6> lane_idx = (ap_int<6>)lane;
      const s16_t d_val = narrow16(d_base - (wide_t)(lane_idx * m0));
      lane_sum[lane] += (int)d_val;
    }

    d_base -= (wide_t)((ap_int<6>)OUT_UNROLL_FACTOR * m0);
  }

  int local_sum = 0;
  for (int lane = 0; lane < OUT_UNROLL_FACTOR; ++lane) {
    #pragma HLS UNROLL
    local_sum += lane_sum[lane];
  }

  return local_sum;
}

void kernel_2mm(short seed, int *sum)
{
  int row_lane_sum[OUT_UNROLL_FACTOR];
  #pragma HLS ARRAY_PARTITION variable=row_lane_sum complete dim=1

  for (int lane = 0; lane < OUT_UNROLL_FACTOR; ++lane) {
    #pragma HLS UNROLL
    row_lane_sum[lane] = 0;
  }

  for (int i = 0; i < NI; ++i) {
    m0_t m0 = 0;
    m1_t m1 = 0;

    compute_row_moments(i, seed, &m0, &m1);
    row_lane_sum[i % OUT_UNROLL_FACTOR] += consume_row_recurrence(seed, m0, m1);
  }

  int local_sum = 0;
  for (int lane = 0; lane < OUT_UNROLL_FACTOR; ++lane) {
    #pragma HLS UNROLL
    local_sum += row_lane_sum[lane];
  }

  *sum = local_sum;
}
