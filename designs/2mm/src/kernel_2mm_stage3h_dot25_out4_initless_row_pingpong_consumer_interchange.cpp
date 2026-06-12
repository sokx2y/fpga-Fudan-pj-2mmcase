#include "kernel_2mm.h"

// Stage 3H: v3g timing-relief base plus consumer loop interchange.
// The top-level row ping-pong DATAFLOW and compute_tmp_row are kept from v3g.
// Only consume_tmp_row is changed to scan k-blocks outside output tiles:
//   kg0: jt0..jt24, kg1: jt0..jt24, ...
// This keeps only a current-row D accumulator, not the full D matrix.
#define DOT_UNROLL_FACTOR 25
#define OUT_UNROLL_FACTOR 4
#define OUT_TILE_COUNT (NL / OUT_UNROLL_FACTOR)
#define K_BLOCK_COUNT (NJ / DOT_UNROLL_FACTOR)

static DATA_TYPE make_a(int i, int k, short seed)
{
  return (DATA_TYPE)(i + k + seed);
}

static DATA_TYPE make_b(int k, int j, short seed)
{
  return (DATA_TYPE)(k + j - seed);
}

static DATA_TYPE make_c(int k, int j, short seed)
{
  return (DATA_TYPE)(k - j + seed);
}

static DATA_TYPE add_product(DATA_TYPE acc, DATA_TYPE lhs, DATA_TYPE rhs)
{
  #pragma HLS INLINE
  return (DATA_TYPE)(acc + lhs * rhs);
}

static DATA_TYPE add_short(DATA_TYPE lhs, DATA_TYPE rhs)
{
  #pragma HLS INLINE
  return (DATA_TYPE)(lhs + rhs);
}

static void compute_tmp_row(int i, short seed, DATA_TYPE tmp_row[NJ])
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp_row cyclic factor=25 dim=1

  const int i_local = i;
  const short seed_local = seed;

  for (int j0 = 0; j0 < NJ; j0 += OUT_UNROLL_FACTOR) {
    #pragma HLS PIPELINE II=1
    DATA_TYPE acc[OUT_UNROLL_FACTOR];
    DATA_TYPE b_lane[OUT_UNROLL_FACTOR];
    #pragma HLS ARRAY_PARTITION variable=acc complete dim=1
    #pragma HLS ARRAY_PARTITION variable=b_lane complete dim=1

    const int j_base = j0;

    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      acc[jo] = 0;
    }

    for (int k = 0; k < NK; ++k) {
      #pragma HLS UNROLL factor=25
      const int k_local = k;
      const DATA_TYPE a_lane = make_a(i_local, k_local, seed_local);

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        b_lane[jo] = make_b(k_local, j_base + jo, seed_local);
      }

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        acc[jo] = add_product(acc[jo], a_lane, b_lane[jo]);
      }
    }

    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      tmp_row[j_base + jo] = acc[jo];
    }
  }
}

static void consume_tmp_row(
    int i,
    short seed,
    DATA_TYPE tmp_row[NJ],
    int lane_sum[OUT_UNROLL_FACTOR])
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp_row cyclic factor=25 dim=1
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=1

  DATA_TYPE d_acc[OUT_TILE_COUNT][OUT_UNROLL_FACTOR];
  #pragma HLS ARRAY_PARTITION variable=d_acc complete dim=2

  const short seed_local = seed;
  (void)i;

  for (int kg = 0; kg < K_BLOCK_COUNT; ++kg) {
    for (int jt = 0; jt < OUT_TILE_COUNT; ++jt) {
      #pragma HLS PIPELINE II=1
      DATA_TYPE group_sum[OUT_UNROLL_FACTOR];
      #pragma HLS ARRAY_PARTITION variable=group_sum complete dim=1

      const int k_base = kg * DOT_UNROLL_FACTOR;
      const int j_base = jt * OUT_UNROLL_FACTOR;

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        group_sum[jo] = 0;
      }

      for (int kk = 0; kk < DOT_UNROLL_FACTOR; ++kk) {
        #pragma HLS UNROLL
        const int k = k_base + kk;
        const DATA_TYPE tmp_lane = tmp_row[k];

        for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
          #pragma HLS UNROLL
          const DATA_TYPE c_lane = make_c(k, j_base + jo, seed_local);
          group_sum[jo] = add_product(group_sum[jo], tmp_lane, c_lane);
        }
      }

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        if (kg == 0) {
          d_acc[jt][jo] = group_sum[jo];
        } else {
          d_acc[jt][jo] = add_short(d_acc[jt][jo], group_sum[jo]);
        }
      }
    }
  }

  for (int jt = 0; jt < OUT_TILE_COUNT; ++jt) {
    #pragma HLS PIPELINE II=1
    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      lane_sum[jo] += d_acc[jt][jo];
    }
  }
}

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE tmp_ping[NJ];
  DATA_TYPE tmp_pong[NJ];
  int lane_sum[OUT_UNROLL_FACTOR];

  #pragma HLS ARRAY_PARTITION variable=tmp_ping cyclic factor=25 dim=1
  #pragma HLS ARRAY_PARTITION variable=tmp_pong cyclic factor=25 dim=1
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=1

  for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
    #pragma HLS UNROLL
    lane_sum[jo] = 0;
  }

  compute_tmp_row(0, seed, tmp_ping);

  for (int i = 1; i < NI; ++i) {
    if ((i & 1) != 0) {
      {
        #pragma HLS DATAFLOW
        compute_tmp_row(i, seed, tmp_pong);
        consume_tmp_row(i - 1, seed, tmp_ping, lane_sum);
      }
    } else {
      {
        #pragma HLS DATAFLOW
        compute_tmp_row(i, seed, tmp_ping);
        consume_tmp_row(i - 1, seed, tmp_pong, lane_sum);
      }
    }
  }

  consume_tmp_row(NI - 1, seed, ((NI & 1) != 0) ? tmp_ping : tmp_pong, lane_sum);

  int local_sum = 0;
  for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
    #pragma HLS UNROLL
    local_sum += lane_sum[jo];
  }

  *sum = local_sum;
}
