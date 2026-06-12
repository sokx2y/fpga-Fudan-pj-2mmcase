#include "kernel_2mm.h"

#ifdef __SYNTHESIS__
#include <hls_stream.h>
#else
#include <queue>

namespace hls {
template <typename T>
class stream {
public:
  stream() {}
  explicit stream(const char *) {}

  void write(const T &value) { values.push(value); }

  T read()
  {
    T value = values.front();
    values.pop();
    return value;
  }

private:
  std::queue<T> values;
};
}
#endif

// Stage 3C: DOT=25, OUT=4, no full D, row-streamed tmp.
// Producer computes tmp rows and streams them to the GEMM2 consumer.
#define DOT_UNROLL_FACTOR 25
#define OUT_UNROLL_FACTOR 4
#define TMP_STREAM_DEPTH 128

static void init_A(short seed, DATA_TYPE A[NI][NK])
{
  for (int i = 0; i < NI; i++) {
    for (int j = 0; j < NK; j++) {
      A[i][j] = i + j + seed;
    }
  }
}

static void init_B(short seed, DATA_TYPE B[NK][NJ])
{
  for (int i = 0; i < NK; i++) {
    for (int j = 0; j < NJ; j++) {
      B[i][j] = i + j - seed;
    }
  }
}

static void init_C(short seed, DATA_TYPE C[NJ][NL])
{
  for (int i = 0; i < NJ; i++) {
    for (int j = 0; j < NL; j++) {
      C[i][j] = i - j + seed;
    }
  }
}

static void produce_tmp_rows(
    DATA_TYPE A[NI][NK],
    DATA_TYPE B[NK][NJ],
    hls::stream<DATA_TYPE> &tmp_stream)
{
  for (int i = 0; i < NI; i++) {
    for (int j0 = 0; j0 < NJ; j0 += OUT_UNROLL_FACTOR) {
      #pragma HLS PIPELINE II=1
      DATA_TYPE acc[OUT_UNROLL_FACTOR];
      #pragma HLS ARRAY_PARTITION variable=acc complete dim=1

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        acc[jo] = 0;
      }

      for (int k = 0; k < NK; ++k) {
        #pragma HLS UNROLL factor=25
        for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
          #pragma HLS UNROLL
          const int j = j0 + jo;
          if (j < NJ) {
            acc[jo] += A[i][k] * B[k][j];
          }
        }
      }

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        const int j = j0 + jo;
        if (j < NJ) {
          tmp_stream.write(acc[jo]);
        }
      }
    }
  }
}

static void consume_tmp_rows(
    DATA_TYPE C[NJ][NL],
    hls::stream<DATA_TYPE> &tmp_stream,
    hls::stream<int> &sum_stream)
{
  int lane_sum[OUT_UNROLL_FACTOR];
  #pragma HLS ARRAY_PARTITION variable=lane_sum complete dim=1

  for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
    #pragma HLS UNROLL
    lane_sum[jo] = 0;
  }

  for (int i = 0; i < NI; i++) {
    DATA_TYPE tmp_row[NJ];
    #pragma HLS ARRAY_PARTITION variable=tmp_row cyclic factor=25 dim=1

    for (int k = 0; k < NJ; ++k) {
      #pragma HLS PIPELINE II=1
      tmp_row[k] = tmp_stream.read();
    }

    for (int j0 = 0; j0 < NL; j0 += OUT_UNROLL_FACTOR) {
      #pragma HLS PIPELINE II=1
      DATA_TYPE acc[OUT_UNROLL_FACTOR];
      #pragma HLS ARRAY_PARTITION variable=acc complete dim=1

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        acc[jo] = 0;
      }

      for (int k = 0; k < NJ; ++k) {
        #pragma HLS UNROLL factor=25
        for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
          #pragma HLS UNROLL
          const int j = j0 + jo;
          if (j < NL) {
            acc[jo] += tmp_row[k] * C[k][j];
          }
        }
      }

      for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
        #pragma HLS UNROLL
        const int j = j0 + jo;
        if (j < NL) {
          lane_sum[jo] += acc[jo];
        }
      }
    }
  }

  int local_sum = 0;
  for (int jo = 0; jo < OUT_UNROLL_FACTOR; ++jo) {
    #pragma HLS UNROLL
    local_sum += lane_sum[jo];
  }

  sum_stream.write(local_sum);
}

static void write_sum(hls::stream<int> &sum_stream, int *sum)
{
  *sum = sum_stream.read();
}

void kernel_2mm(short seed, int *sum)
{
  DATA_TYPE A[NI][NK];
  DATA_TYPE B[NK][NJ];
  DATA_TYPE C[NJ][NL];
  hls::stream<DATA_TYPE> tmp_stream("tmp_stream");
  hls::stream<int> sum_stream("sum_stream");

  #pragma HLS ARRAY_PARTITION variable=A cyclic factor=25 dim=2
  #pragma HLS ARRAY_PARTITION variable=B cyclic factor=25 dim=1
  #pragma HLS ARRAY_PARTITION variable=C cyclic factor=25 dim=1
  #pragma HLS ARRAY_PARTITION variable=B cyclic factor=4 dim=2
  #pragma HLS ARRAY_PARTITION variable=C cyclic factor=4 dim=2
  #pragma HLS STREAM variable=tmp_stream depth=128
  #pragma HLS STREAM variable=sum_stream depth=2
  #pragma HLS DATAFLOW

  init_A(seed, A);
  init_B(seed, B);
  init_C(seed, C);
  produce_tmp_rows(A, B, tmp_stream);
  consume_tmp_rows(C, tmp_stream, sum_stream);
  write_sum(sum_stream, sum);
}
