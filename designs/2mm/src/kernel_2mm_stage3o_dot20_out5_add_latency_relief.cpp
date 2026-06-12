#include "kernel_2mm.h"

// Stage 3O: v3n DOT20 / OUT5 with explicit short-add timing relief.
//
// v3n keeps the fast DOT20 / OUT5 row-pingpong shape, but Vivado 4.6 ns moved
// the worst paths into compute-side short accumulation:
//   add_ln29 register -> CARRY4 chain -> add_ln29 register.
//
// This variant keeps the same algorithmic/dataflow structure and asks HLS to
// schedule the short accumulator add with latency 2 in fabric.  The intent is
// to trade a small number of cycles for a shorter routed carry path.
#define DOT_UNROLL_FACTOR 20
#define OUT_UNROLL_FACTOR 5

static DATA_TYPE mul_dsp_short(DATA_TYPE lhs, DATA_TYPE rhs)
{
  #pragma HLS INLINE
  DATA_TYPE product = (DATA_TYPE)(lhs * rhs);
  #pragma HLS bind_op variable=product op=mul impl=dsp latency=2
  return product;
}

static DATA_TYPE add_product(DATA_TYPE acc, DATA_TYPE lhs, DATA_TYPE rhs)
{
  #pragma HLS INLINE
  const DATA_TYPE product = mul_dsp_short(lhs, rhs);
  DATA_TYPE sum = (DATA_TYPE)(acc + product);
  #pragma HLS bind_op variable=sum op=add impl=fabric latency=2
  return sum;
}

static DATA_TYPE add_short(DATA_TYPE lhs, DATA_TYPE rhs)
{
  #pragma HLS INLINE
  return (DATA_TYPE)(lhs + rhs);
}

static void compute_tmp_row(int i, short seed, DATA_TYPE tmp_row[NJ])
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp_row complete dim=1

  const DATA_TYPE i_plus_seed = (DATA_TYPE)(i + seed);
  const DATA_TYPE seed_local = (DATA_TYPE)seed;

  for (int j0 = 0; j0 < NJ; j0 += OUT_UNROLL_FACTOR) {
    #pragma HLS PIPELINE II=1
    DATA_TYPE acc[OUT_UNROLL_FACTOR];
    DATA_TYPE b_lane[OUT_UNROLL_FACTOR];
    DATA_TYPE j_minus_seed[OUT_UNROLL_FACTOR];
    #pragma HLS ARRAY_PARTITION variable=acc complete dim=1
    #pragma HLS ARRAY_PARTITION variable=b_lane complete dim=1
    #pragma HLS ARRAY_PARTITION variable=j_minus_seed complete dim=1

    const int j_base = j0;

    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      acc[jo] = 0;
      j_minus_seed[jo] = (DATA_TYPE)((DATA_TYPE)(j_base + jo) - seed_local);
    }

    for (int k = 0; k < NK; ++k) {
      #pragma HLS UNROLL factor=DOT_UNROLL_FACTOR
      const DATA_TYPE k_local = (DATA_TYPE)k;
      const DATA_TYPE a_lane = add_short(i_plus_seed, k_local);

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        b_lane[jo] = add_short(k_local, j_minus_seed[jo]);
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
  #pragma HLS ARRAY_PARTITION variable=tmp_row complete dim=1
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=1

  const DATA_TYPE seed_local = (DATA_TYPE)seed;
  (void)i;

  for (int j0 = 0; j0 < NL; j0 += OUT_UNROLL_FACTOR) {
    #pragma HLS PIPELINE II=1
    DATA_TYPE acc[OUT_UNROLL_FACTOR];
    DATA_TYPE c_lane[OUT_UNROLL_FACTOR];
    DATA_TYPE seed_minus_j[OUT_UNROLL_FACTOR];
    #pragma HLS ARRAY_PARTITION variable=acc complete dim=1
    #pragma HLS ARRAY_PARTITION variable=c_lane complete dim=1
    #pragma HLS ARRAY_PARTITION variable=seed_minus_j complete dim=1

    const int j_base = j0;

    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      acc[jo] = 0;
      seed_minus_j[jo] = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + jo));
    }

    for (int k = 0; k < NJ; ++k) {
      #pragma HLS UNROLL factor=DOT_UNROLL_FACTOR
      const DATA_TYPE k_local = (DATA_TYPE)k;
      const DATA_TYPE tmp_lane = tmp_row[k];

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        c_lane[jo] = add_short(k_local, seed_minus_j[jo]);
      }

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        acc[jo] = add_product(acc[jo], tmp_lane, c_lane[jo]);
      }
    }

    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      lane_sum[jo] += acc[jo];
    }
  }
}

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE tmp_ping[NJ];
  DATA_TYPE tmp_pong[NJ];
  int lane_sum[OUT_UNROLL_FACTOR];

  #pragma HLS ARRAY_PARTITION variable=tmp_ping complete dim=1
  #pragma HLS ARRAY_PARTITION variable=tmp_pong complete dim=1
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
