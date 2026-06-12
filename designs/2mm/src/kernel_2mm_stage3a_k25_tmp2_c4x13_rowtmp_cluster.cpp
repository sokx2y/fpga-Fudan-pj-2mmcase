#include "kernel_2mm.h"

// Stage 3A stable candidate, revised:
// - GEMM1 row producer: K_LANES=25, TMP_LANES=2.
// - GEMM2 row consumer: 4 explicit C-column clusters.
// - Each cluster computes one output-column dot-product with 13 k-lanes.
// - No full tmp and no full D storage.
#define K_LANES 25
#define TMP_LANES 2
#define C_CLUSTERS 4
#define C_CLUSTER_COLS 25
#define C_K_LANES 13

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE A[NI][NK];
  DATA_TYPE B[NK][NJ];
  DATA_TYPE C0[NJ][C_CLUSTER_COLS];
  DATA_TYPE C1[NJ][C_CLUSTER_COLS];
  DATA_TYPE C2[NJ][C_CLUSTER_COLS];
  DATA_TYPE C3[NJ][C_CLUSTER_COLS];

  #pragma HLS ARRAY_PARTITION variable=A cyclic factor=K_LANES dim=2
  #pragma HLS ARRAY_PARTITION variable=B cyclic factor=K_LANES dim=1
  #pragma HLS ARRAY_PARTITION variable=B cyclic factor=TMP_LANES dim=2
  #pragma HLS ARRAY_PARTITION variable=C0 cyclic factor=C_K_LANES dim=1
  #pragma HLS ARRAY_PARTITION variable=C1 cyclic factor=C_K_LANES dim=1
  #pragma HLS ARRAY_PARTITION variable=C2 cyclic factor=C_K_LANES dim=1
  #pragma HLS ARRAY_PARTITION variable=C3 cyclic factor=C_K_LANES dim=1

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
    for (int j = 0; j < C_CLUSTER_COLS; j++) {
      #pragma HLS PIPELINE II=1
      C0[i][j] = i - j + seed;
      C1[i][j] = i - (j + 25) + seed;
      C2[i][j] = i - (j + 50) + seed;
      C3[i][j] = i - (j + 75) + seed;
    }
  }

  int cluster_sum[C_CLUSTERS];
  #pragma HLS ARRAY_PARTITION variable=cluster_sum complete dim=1

  for (int c = 0; c < C_CLUSTERS; ++c) {
    #pragma HLS UNROLL
    cluster_sum[c] = 0;
  }

  for (int i = 0; i < NI; i++) {
    DATA_TYPE tmp_row[NJ];
    #pragma HLS ARRAY_PARTITION variable=tmp_row cyclic factor=C_K_LANES dim=1

    for (int j0 = 0; j0 < NJ; j0 += TMP_LANES) {
      #pragma HLS PIPELINE II=1
      DATA_TYPE acc[TMP_LANES];
      #pragma HLS ARRAY_PARTITION variable=acc complete dim=1

      for (int jo = 0; jo < TMP_LANES; ++jo) {
        #pragma HLS UNROLL
        acc[jo] = 0;
      }

      for (int k = 0; k < NK; ++k) {
        #pragma HLS UNROLL factor=K_LANES
        for (int jo = 0; jo < TMP_LANES; ++jo) {
          #pragma HLS UNROLL
          const int j = j0 + jo;
          if (j < NJ) {
            acc[jo] += A[i][k] * B[k][j];
          }
        }
      }

      for (int jo = 0; jo < TMP_LANES; ++jo) {
        #pragma HLS UNROLL
        const int j = j0 + jo;
        if (j < NJ) {
          tmp_row[j] = acc[jo];
        }
      }
    }

    for (int j = 0; j < C_CLUSTER_COLS; ++j) {
      DATA_TYPE acc0 = 0;
      DATA_TYPE acc1 = 0;
      DATA_TYPE acc2 = 0;
      DATA_TYPE acc3 = 0;

      for (int k0 = 0; k0 < NJ; k0 += C_K_LANES) {
        #pragma HLS PIPELINE II=1
        for (int lane = 0; lane < C_K_LANES; ++lane) {
          #pragma HLS UNROLL
          const int k = k0 + lane;
          if (k < NJ) {
            const DATA_TYPE tmp_val = tmp_row[k];
            acc0 += tmp_val * C0[k][j];
            acc1 += tmp_val * C1[k][j];
            acc2 += tmp_val * C2[k][j];
            acc3 += tmp_val * C3[k][j];
          }
        }
      }

      cluster_sum[0] += acc0;
      cluster_sum[1] += acc1;
      cluster_sum[2] += acc2;
      cluster_sum[3] += acc3;
    }
  }

  int local_sum = 0;
  for (int c = 0; c < C_CLUSTERS; ++c) {
    #pragma HLS UNROLL
    local_sum += cluster_sum[c];
  }

  *sum = local_sum;
}
