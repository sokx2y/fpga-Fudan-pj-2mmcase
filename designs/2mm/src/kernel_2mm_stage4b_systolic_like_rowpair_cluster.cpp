#include "kernel_2mm.h"
#include <ap_int.h>

// Stage 4B: systolic-like row-pair PE cluster experiment.
//
// This is the high-risk V4 branch after v3q/v4a.  It borrows the GEMM-HLS
// row-stationary / PE-cluster idea, but scales it to the fixed 100x100 short
// 2mm case and the XC7K325T DSP limit.
//
// Lessons carried forward from V3:
//   - keep initless generated A/B/C values;
//   - keep no full D store and accumulate only the final sum;
//   - preserve short product truncation and short tmp/D truncation;
//   - keep local ap_int<24> dot accumulators;
//   - avoid latency-binding the accumulator recurrence;
//   - do not blindly double DOT20/OUT5, because v3q already sits at 840 DSP.
//
// V4B shape:
//   ROW_TILE=2, DOT10, OUT5
//   Each output cluster computes two adjacent rows.  B/C operands are shared
//   inside the row-pair cluster, while A/tmp operands stay row-local.  The
//   logical work per compute/consume phase stays near the v3q 100-product
//   active shape: 2 rows * 5 outputs * 10 k-lanes.
//
// Important HLS scheduling guard:
//   Do not pipeline the OUT5 j-tile loop around fully inlined cluster bodies.
//   Vitis HLS 2023.2 may then imply a complete K=100 unroll while trying to
//   satisfy the outer II=1 request.  Keep cluster functions out-of-line and
//   pipeline the K chunk loop inside each cluster instead.
#define ROW_TILE_FACTOR 2
#define DOT_UNROLL_FACTOR 10
#define OUT_UNROLL_FACTOR 5
#define K_CHUNK_COUNT (NK / DOT_UNROLL_FACTOR)

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
static void compute_tmp_rowpair_cluster(
    DATA_TYPE i0_plus_seed,
    DATA_TYPE i1_plus_seed,
    DATA_TYPE j_minus_seed,
    DATA_TYPE &tmp0_out,
    DATA_TYPE &tmp1_out)
{
  #pragma HLS INLINE off
  dot_acc_t acc0 = 0;
  dot_acc_t acc1 = 0;

  for (int kc = 0; kc < K_CHUNK_COUNT; ++kc) {
    #pragma HLS PIPELINE II=1
    dot_acc_t chunk0 = 0;
    dot_acc_t chunk1 = 0;

    for (int kk = 0; kk < DOT_UNROLL_FACTOR; ++kk) {
      #pragma HLS UNROLL
      const int k = kc * DOT_UNROLL_FACTOR + kk;
      const DATA_TYPE k_local = (DATA_TYPE)k;
      const DATA_TYPE b_reg = add_short(k_local, j_minus_seed);
      const DATA_TYPE a0_reg = add_short(i0_plus_seed, k_local);
      const DATA_TYPE a1_reg = add_short(i1_plus_seed, k_local);

      chunk0 = add_product_narrow(chunk0, a0_reg, b_reg);
      chunk1 = add_product_narrow(chunk1, a1_reg, b_reg);
    }

    acc0 += chunk0;
    acc1 += chunk1;
  }

  tmp0_out = (DATA_TYPE)acc0;
  tmp1_out = (DATA_TYPE)acc1;
}

template <int CLUSTER_ID>
static void consume_tmp_rowpair_cluster(
    DATA_TYPE seed_minus_j,
    DATA_TYPE tmp0_row[NJ],
    DATA_TYPE tmp1_row[NJ],
    DATA_TYPE &d0_out,
    DATA_TYPE &d1_out)
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp0_row complete dim=1
  #pragma HLS ARRAY_PARTITION variable=tmp1_row complete dim=1

  dot_acc_t acc0 = 0;
  dot_acc_t acc1 = 0;

  for (int kc = 0; kc < K_CHUNK_COUNT; ++kc) {
    #pragma HLS PIPELINE II=1
    dot_acc_t chunk0 = 0;
    dot_acc_t chunk1 = 0;

    for (int kk = 0; kk < DOT_UNROLL_FACTOR; ++kk) {
      #pragma HLS UNROLL
      const int k = kc * DOT_UNROLL_FACTOR + kk;
      const DATA_TYPE k_local = (DATA_TYPE)k;
      const DATA_TYPE c_reg = add_short(k_local, seed_minus_j);
      const DATA_TYPE tmp0_reg = tmp0_row[k];
      const DATA_TYPE tmp1_reg = tmp1_row[k];

      chunk0 = add_product_narrow(chunk0, tmp0_reg, c_reg);
      chunk1 = add_product_narrow(chunk1, tmp1_reg, c_reg);
    }

    acc0 += chunk0;
    acc1 += chunk1;
  }

  d0_out = (DATA_TYPE)acc0;
  d1_out = (DATA_TYPE)acc1;
}

static void compute_tmp_rowpair(
    int i_base,
    short seed,
    DATA_TYPE tmp0_row[NJ],
    DATA_TYPE tmp1_row[NJ])
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp0_row complete dim=1
  #pragma HLS ARRAY_PARTITION variable=tmp1_row complete dim=1

  const DATA_TYPE i0_plus_seed = (DATA_TYPE)(i_base + seed);
  const DATA_TYPE i1_plus_seed = (DATA_TYPE)(i_base + 1 + seed);
  const DATA_TYPE seed_local = (DATA_TYPE)seed;

  for (int j0 = 0; j0 < NJ; j0 += OUT_UNROLL_FACTOR) {
    const int j_base = j0;

    const DATA_TYPE jm0 = (DATA_TYPE)((DATA_TYPE)(j_base + 0) - seed_local);
    const DATA_TYPE jm1 = (DATA_TYPE)((DATA_TYPE)(j_base + 1) - seed_local);
    const DATA_TYPE jm2 = (DATA_TYPE)((DATA_TYPE)(j_base + 2) - seed_local);
    const DATA_TYPE jm3 = (DATA_TYPE)((DATA_TYPE)(j_base + 3) - seed_local);
    const DATA_TYPE jm4 = (DATA_TYPE)((DATA_TYPE)(j_base + 4) - seed_local);

    compute_tmp_rowpair_cluster<0>(
        i0_plus_seed, i1_plus_seed, jm0, tmp0_row[j_base + 0], tmp1_row[j_base + 0]);
    compute_tmp_rowpair_cluster<1>(
        i0_plus_seed, i1_plus_seed, jm1, tmp0_row[j_base + 1], tmp1_row[j_base + 1]);
    compute_tmp_rowpair_cluster<2>(
        i0_plus_seed, i1_plus_seed, jm2, tmp0_row[j_base + 2], tmp1_row[j_base + 2]);
    compute_tmp_rowpair_cluster<3>(
        i0_plus_seed, i1_plus_seed, jm3, tmp0_row[j_base + 3], tmp1_row[j_base + 3]);
    compute_tmp_rowpair_cluster<4>(
        i0_plus_seed, i1_plus_seed, jm4, tmp0_row[j_base + 4], tmp1_row[j_base + 4]);
  }
}

static void consume_tmp_rowpair(
    int i_base,
    short seed,
    DATA_TYPE tmp0_row[NJ],
    DATA_TYPE tmp1_row[NJ],
    int lane_sum[ROW_TILE_FACTOR][OUT_UNROLL_FACTOR])
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp0_row complete dim=1
  #pragma HLS ARRAY_PARTITION variable=tmp1_row complete dim=1
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=0

  const DATA_TYPE seed_local = (DATA_TYPE)seed;
  (void)i_base;

  for (int j0 = 0; j0 < NL; j0 += OUT_UNROLL_FACTOR) {
    const int j_base = j0;

    const DATA_TYPE sm0 = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + 0));
    const DATA_TYPE sm1 = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + 1));
    const DATA_TYPE sm2 = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + 2));
    const DATA_TYPE sm3 = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + 3));
    const DATA_TYPE sm4 = (DATA_TYPE)(seed_local - (DATA_TYPE)(j_base + 4));

    DATA_TYPE d00;
    DATA_TYPE d01;
    DATA_TYPE d10;
    DATA_TYPE d11;
    DATA_TYPE d20;
    DATA_TYPE d21;
    DATA_TYPE d30;
    DATA_TYPE d31;
    DATA_TYPE d40;
    DATA_TYPE d41;

    consume_tmp_rowpair_cluster<0>(sm0, tmp0_row, tmp1_row, d00, d01);
    consume_tmp_rowpair_cluster<1>(sm1, tmp0_row, tmp1_row, d10, d11);
    consume_tmp_rowpair_cluster<2>(sm2, tmp0_row, tmp1_row, d20, d21);
    consume_tmp_rowpair_cluster<3>(sm3, tmp0_row, tmp1_row, d30, d31);
    consume_tmp_rowpair_cluster<4>(sm4, tmp0_row, tmp1_row, d40, d41);

    lane_sum[0][0] += d00;
    lane_sum[1][0] += d01;
    lane_sum[0][1] += d10;
    lane_sum[1][1] += d11;
    lane_sum[0][2] += d20;
    lane_sum[1][2] += d21;
    lane_sum[0][3] += d30;
    lane_sum[1][3] += d31;
    lane_sum[0][4] += d40;
    lane_sum[1][4] += d41;
  }
}

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE tmp_ping0[NJ];
  DATA_TYPE tmp_ping1[NJ];
  DATA_TYPE tmp_pong0[NJ];
  DATA_TYPE tmp_pong1[NJ];
  int lane_sum[ROW_TILE_FACTOR][OUT_UNROLL_FACTOR];

  #pragma HLS ARRAY_PARTITION variable=tmp_ping0 complete dim=1
  #pragma HLS ARRAY_PARTITION variable=tmp_ping1 complete dim=1
  #pragma HLS ARRAY_PARTITION variable=tmp_pong0 complete dim=1
  #pragma HLS ARRAY_PARTITION variable=tmp_pong1 complete dim=1
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=0

  for (int r = 0; r < ROW_TILE_FACTOR; ++r) {
    #pragma HLS UNROLL
    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      lane_sum[r][jo] = 0;
    }
  }

  compute_tmp_rowpair(0, seed, tmp_ping0, tmp_ping1);

  for (int pair = 1; pair < (NI / ROW_TILE_FACTOR); ++pair) {
    const int i_base = pair * ROW_TILE_FACTOR;

    if ((pair & 1) != 0) {
      {
        #pragma HLS DATAFLOW
        compute_tmp_rowpair(i_base, seed, tmp_pong0, tmp_pong1);
        consume_tmp_rowpair(i_base - ROW_TILE_FACTOR, seed, tmp_ping0, tmp_ping1, lane_sum);
      }
    } else {
      {
        #pragma HLS DATAFLOW
        compute_tmp_rowpair(i_base, seed, tmp_ping0, tmp_ping1);
        consume_tmp_rowpair(i_base - ROW_TILE_FACTOR, seed, tmp_pong0, tmp_pong1, lane_sum);
      }
    }
  }

  consume_tmp_rowpair(
      NI - ROW_TILE_FACTOR,
      seed,
      tmp_pong0,
      tmp_pong1,
      lane_sum);

  int local_sum = 0;
  for (int r = 0; r < ROW_TILE_FACTOR; ++r) {
    #pragma HLS UNROLL
    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      local_sum += lane_sum[r][jo];
    }
  }

  *sum = local_sum;
}
