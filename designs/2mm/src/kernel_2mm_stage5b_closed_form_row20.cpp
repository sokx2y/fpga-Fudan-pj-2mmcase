#include "kernel_2mm.h"
#include <ap_int.h>

// Stage 5B: closed-form / moment recurrence with row-level parallelism.
//
// V5A proved the closed-form path but serialized all 100 rows.  This version
// spends DSP/LUT/FF resources by processing 20 independent rows in parallel.
#define MOMENT_UNROLL_FACTOR 20
#define OUT_UNROLL_FACTOR 20
#define ROW_UNROLL_FACTOR 20

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

template <int ROW_LANE>
static void compute_row_moments_lane(int i, short seed, m0_t *m0, m1_t *m1)
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

template <int ROW_LANE>
static int consume_row_recurrence_lane(short seed, m0_t m0, m1_t m1)
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

template <int ROW_LANE>
static int compute_row_sum_lane(int i, short seed)
{
  #pragma HLS INLINE off

  m0_t m0 = 0;
  m1_t m1 = 0;
  compute_row_moments_lane<ROW_LANE>(i, seed, &m0, &m1);
  return consume_row_recurrence_lane<ROW_LANE>(seed, m0, m1);
}

void kernel_2mm(short seed, int *sum)
{
  int row_sum[ROW_UNROLL_FACTOR];
  #pragma HLS ARRAY_PARTITION variable=row_sum complete dim=1

  for (int lane = 0; lane < ROW_UNROLL_FACTOR; ++lane) {
    #pragma HLS UNROLL
    row_sum[lane] = 0;
  }

  for (int i0 = 0; i0 < NI; i0 += ROW_UNROLL_FACTOR) {
    int s0;
    int s1;
    int s2;
    int s3;
    int s4;
    int s5;
    int s6;
    int s7;
    int s8;
    int s9;
    int s10;
    int s11;
    int s12;
    int s13;
    int s14;
    int s15;
    int s16;
    int s17;
    int s18;
    int s19;

    {
      #pragma HLS DATAFLOW
      s0 = compute_row_sum_lane<0>(i0 + 0, seed);
      s1 = compute_row_sum_lane<1>(i0 + 1, seed);
      s2 = compute_row_sum_lane<2>(i0 + 2, seed);
      s3 = compute_row_sum_lane<3>(i0 + 3, seed);
      s4 = compute_row_sum_lane<4>(i0 + 4, seed);
      s5 = compute_row_sum_lane<5>(i0 + 5, seed);
      s6 = compute_row_sum_lane<6>(i0 + 6, seed);
      s7 = compute_row_sum_lane<7>(i0 + 7, seed);
      s8 = compute_row_sum_lane<8>(i0 + 8, seed);
      s9 = compute_row_sum_lane<9>(i0 + 9, seed);
      s10 = compute_row_sum_lane<10>(i0 + 10, seed);
      s11 = compute_row_sum_lane<11>(i0 + 11, seed);
      s12 = compute_row_sum_lane<12>(i0 + 12, seed);
      s13 = compute_row_sum_lane<13>(i0 + 13, seed);
      s14 = compute_row_sum_lane<14>(i0 + 14, seed);
      s15 = compute_row_sum_lane<15>(i0 + 15, seed);
      s16 = compute_row_sum_lane<16>(i0 + 16, seed);
      s17 = compute_row_sum_lane<17>(i0 + 17, seed);
      s18 = compute_row_sum_lane<18>(i0 + 18, seed);
      s19 = compute_row_sum_lane<19>(i0 + 19, seed);
    }

    row_sum[0] += s0;
    row_sum[1] += s1;
    row_sum[2] += s2;
    row_sum[3] += s3;
    row_sum[4] += s4;
    row_sum[5] += s5;
    row_sum[6] += s6;
    row_sum[7] += s7;
    row_sum[8] += s8;
    row_sum[9] += s9;
    row_sum[10] += s10;
    row_sum[11] += s11;
    row_sum[12] += s12;
    row_sum[13] += s13;
    row_sum[14] += s14;
    row_sum[15] += s15;
    row_sum[16] += s16;
    row_sum[17] += s17;
    row_sum[18] += s18;
    row_sum[19] += s19;
  }

  int local_sum = 0;
  for (int lane = 0; lane < ROW_UNROLL_FACTOR; ++lane) {
    #pragma HLS UNROLL
    local_sum += row_sum[lane];
  }

  *sum = local_sum;
}
