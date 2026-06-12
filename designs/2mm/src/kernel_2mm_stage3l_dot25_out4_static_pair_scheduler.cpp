#include "kernel_2mm.h"

// Stage 3L: v3i datapath with a static two-row ping/pong scheduler.
//
// The v3i 4.6 ns routed checkpoint is already compute-dense:
//   DOT25 / OUT4, initless A/B/C, row ping-pong DATAFLOW, no full tmp,
//   no full D, lane_sum reduction, complete row buffers, and latency-2 DSP
//   multiply binding.
//
// This variant keeps that datapath unchanged and targets top-level row/control
// overhead.  Instead of a per-row runtime if/else selecting ping or pong, the
// top loop processes two rows per iteration with a fixed schedule:
//   odd row  -> pong while consuming previous ping
//   even row -> ping while consuming previous pong
//
// The goal is to see whether HLS/Vivado can reduce row scheduler overhead or
// route-dominated control fanout while preserving v3i's 4238-cycle datapath.
#define DOT_UNROLL_FACTOR 25
#define OUT_UNROLL_FACTOR 4

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
  return (DATA_TYPE)(acc + product);
}

static void compute_tmp_row(int i, short seed, DATA_TYPE tmp_row[NJ])
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp_row complete dim=1

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
    short seed,
    DATA_TYPE tmp_row[NJ],
    int lane_sum[OUT_UNROLL_FACTOR])
{
  #pragma HLS INLINE off
  #pragma HLS ARRAY_PARTITION variable=tmp_row complete dim=1
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=1

  const short seed_local = seed;

  for (int j0 = 0; j0 < NL; j0 += OUT_UNROLL_FACTOR) {
    #pragma HLS PIPELINE II=1
    DATA_TYPE acc[OUT_UNROLL_FACTOR];
    DATA_TYPE c_lane[OUT_UNROLL_FACTOR];
    #pragma HLS ARRAY_PARTITION variable=acc complete dim=1
    #pragma HLS ARRAY_PARTITION variable=c_lane complete dim=1

    const int j_base = j0;

    for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
      #pragma HLS UNROLL
      acc[jo] = 0;
    }

    for (int k = 0; k < NJ; ++k) {
      #pragma HLS UNROLL factor=25
      const int k_local = k;
      const DATA_TYPE tmp_lane = tmp_row[k_local];

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        c_lane[jo] = make_c(k_local, j_base + jo, seed_local);
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

  // NI is fixed at 100.  Rows 1..98 are processed as 49 static odd/even
  // ping-pong pairs, then row 99 is handled by the fixed tail below.
  for (int i = 1; i < NI - 1; i += 2) {
    {
      #pragma HLS DATAFLOW
      compute_tmp_row(i, seed, tmp_pong);
      consume_tmp_row(seed, tmp_ping, lane_sum);
    }

    {
      #pragma HLS DATAFLOW
      compute_tmp_row(i + 1, seed, tmp_ping);
      consume_tmp_row(seed, tmp_pong, lane_sum);
    }
  }

  {
    #pragma HLS DATAFLOW
    compute_tmp_row(NI - 1, seed, tmp_pong);
    consume_tmp_row(seed, tmp_ping, lane_sum);
  }

  consume_tmp_row(seed, tmp_pong, lane_sum);

  int local_sum = 0;
  for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
    #pragma HLS UNROLL
    local_sum += lane_sum[jo];
  }

  *sum = local_sum;
}
