#include "kernel_2mm.h"
#include <ap_int.h>

// Stage 4A: output-stationary PE cluster experiment.
//
// This is the intended V4 architectural branch after the v3q champion.  It
// keeps the proven row ping-pong schedule, but expresses the DOT20 / OUT5 row
// engines as five explicit output-stationary clusters.  Each cluster owns one
// output accumulator and its local DOT20 lane group, rather than sharing one
// broad cross-lane loop body for all OUT lanes.
//
// Goal:
//   Preserve v3q semantics and the 840-DSP-class work rate while giving HLS and
//   Vivado a more regular, placement-friendly compute structure.
#define DOT_UNROLL_FACTOR 20
#define OUT_UNROLL_FACTOR 5

typedef ap_int<24> dot_acc_t;

static DATA_TYPE add_short(DATA_TYPE lhs, DATA_TYPE rhs)
{
  #pragma HLS INLINE
  return (DATA_TYPE)(lhs + rhs);
}

static DATA_TYPE mul_dsp_short(DATA_TYPE lhs, DATA_TYPE rhs)
{
  #pragma HLS INLINE
  DATA_TYPE product = (DATA_TYPE)(lhs * rhs);
  #pragma HLS bind_op variable=product op=mul impl=dsp latency=2
  return product;
}

static dot_acc_t add_product_narrow(dot_acc_t acc, DATA_TYPE lhs, DATA_TYPE rhs)
{
  #pragma HLS INLINE
  const DATA_TYPE product = mul_dsp_short(lhs, rhs);
  return acc + (dot_acc_t)product;
}

template <int CLUSTER_ID>
static dot_acc_t compute_tmp_output_cluster(
    DATA_TYPE i_plus_seed,
    DATA_TYPE j_minus_seed)
{
  #pragma HLS INLINE
  dot_acc_t acc = 0;

  for (int k = 0; k < NK; ++k) {
    #pragma HLS UNROLL factor=DOT_UNROLL_FACTOR
    const DATA_TYPE k_local = (DATA_TYPE)k;
    const DATA_TYPE a_reg = add_short(i_plus_seed, k_local);
    const DATA_TYPE b_reg = add_short(k_local, j_minus_seed);
    acc = add_product_narrow(acc, a_reg, b_reg);
  }

  return acc;
}

template <int CLUSTER_ID>
static DATA_TYPE consume_tmp_output_cluster(
    DATA_TYPE seed_minus_j,
    DATA_TYPE tmp_row[NJ])
{
  #pragma HLS INLINE
  #pragma HLS ARRAY_PARTITION variable=tmp_row complete dim=1

  dot_acc_t acc = 0;

  for (int k = 0; k < NJ; ++k) {
    #pragma HLS UNROLL factor=DOT_UNROLL_FACTOR
    const DATA_TYPE k_local = (DATA_TYPE)k;
    const DATA_TYPE tmp_reg = tmp_row[k];
    const DATA_TYPE c_reg = add_short(k_local, seed_minus_j);
    acc = add_product_narrow(acc, tmp_reg, c_reg);
  }

  return (DATA_TYPE)acc;
}

static void compute_tmp_row(int i, short seed, DATA_TYPE tmp_row[NJ])
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp_row complete dim=1

  const DATA_TYPE i_plus_seed = (DATA_TYPE)(i + seed);
  const DATA_TYPE seed_local = (DATA_TYPE)seed;

  for (int j0 = 0; j0 < NJ; j0 += OUT_UNROLL_FACTOR) {
    #pragma HLS PIPELINE II=1
    const int j_base = j0;

    const DATA_TYPE jm0 = (DATA_TYPE)((DATA_TYPE)(j_base + 0) - seed_local);
    const DATA_TYPE jm1 = (DATA_TYPE)((DATA_TYPE)(j_base + 1) - seed_local);
    const DATA_TYPE jm2 = (DATA_TYPE)((DATA_TYPE)(j_base + 2) - seed_local);
    const DATA_TYPE jm3 = (DATA_TYPE)((DATA_TYPE)(j_base + 3) - seed_local);
    const DATA_TYPE jm4 = (DATA_TYPE)((DATA_TYPE)(j_base + 4) - seed_local);

    const dot_acc_t acc0 = compute_tmp_output_cluster<0>(i_plus_seed, jm0);
    const dot_acc_t acc1 = compute_tmp_output_cluster<1>(i_plus_seed, jm1);
    const dot_acc_t acc2 = compute_tmp_output_cluster<2>(i_plus_seed, jm2);
    const dot_acc_t acc3 = compute_tmp_output_cluster<3>(i_plus_seed, jm3);
    const dot_acc_t acc4 = compute_tmp_output_cluster<4>(i_plus_seed, jm4);

    tmp_row[j_base + 0] = (DATA_TYPE)acc0;
    tmp_row[j_base + 1] = (DATA_TYPE)acc1;
    tmp_row[j_base + 2] = (DATA_TYPE)acc2;
    tmp_row[j_base + 3] = (DATA_TYPE)acc3;
    tmp_row[j_base + 4] = (DATA_TYPE)acc4;
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
    const int j_base = j0;

    const DATA_TYPE sm0 = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + 0));
    const DATA_TYPE sm1 = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + 1));
    const DATA_TYPE sm2 = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + 2));
    const DATA_TYPE sm3 = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + 3));
    const DATA_TYPE sm4 = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + 4));

    const DATA_TYPE d0 = consume_tmp_output_cluster<0>(sm0, tmp_row);
    const DATA_TYPE d1 = consume_tmp_output_cluster<1>(sm1, tmp_row);
    const DATA_TYPE d2 = consume_tmp_output_cluster<2>(sm2, tmp_row);
    const DATA_TYPE d3 = consume_tmp_output_cluster<3>(sm3, tmp_row);
    const DATA_TYPE d4 = consume_tmp_output_cluster<4>(sm4, tmp_row);

    lane_sum[0] += d0;
    lane_sum[1] += d1;
    lane_sum[2] += d2;
    lane_sum[3] += d3;
    lane_sum[4] += d4;
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

  lane_sum[0] = 0;
  lane_sum[1] = 0;
  lane_sum[2] = 0;
  lane_sum[3] = 0;
  lane_sum[4] = 0;

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

  *sum = lane_sum[0] + lane_sum[1] + lane_sum[2] + lane_sum[3] + lane_sum[4];
}
