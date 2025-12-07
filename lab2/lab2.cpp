#include <stdint.h>

#define HEIGHT 256
#define WIDTH 256
#define T1 32
#define T2 96

void imageDiffPosterize(uint8_t A[HEIGHT][WIDTH],
                       uint8_t B[HEIGHT][WIDTH],
                       uint8_t C[HEIGHT][WIDTH]) {

   // Local buffers that we want to implement in BRAM
   static uint8_t A_local[HEIGHT][WIDTH];
   static uint8_t B_local[HEIGHT][WIDTH];
   static uint8_t C_local[HEIGHT][WIDTH];

// AXI Master Interface (m_axi): Enable direct memory access to external RAM with 1024-byte buffer depth
// Provides high-bandwidth off-chip data transfer with slave offset management
#pragma HLS INTERFACE m_axi port=A depth=1024 offset=slave
#pragma HLS INTERFACE m_axi port=B depth=1024 offset=slave
#pragma HLS INTERFACE m_axi port=C depth=1024 offset=slave

// AXI Slave Interface (s_axilite): Control/status port for software to communicate with hardware
// Bundle registers together for efficient address mapping and control signaling
#pragma HLS INTERFACE s_axilite port=A bundle=control
#pragma HLS INTERFACE s_axilite port=B bundle=control
#pragma HLS INTERFACE s_axilite port=C bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

// Bind storage: Force local arrays to use dual-port BRAM (ram_2p) for simultaneous read/write access
// Enables pipelining and parallel operations on BRAM data with low latency
#pragma HLS bind_storage variable=A_local type=ram_2p impl=bram
#pragma HLS bind_storage variable=B_local type=ram_2p impl=bram
#pragma HLS bind_storage variable=C_local type=ram_2p impl=bram

// Partition dim=2 cyclically to create multiple BRAM ports for higher parallel access.
// Cyclic partitioning keeps data in memory while enabling several parallel accesses per cycle.
#pragma HLS array_partition variable=A_local cyclic factor=256 dim=2
#pragma HLS array_partition variable=B_local cyclic factor=256 dim=2
#pragma HLS array_partition variable=C_local cyclic factor=256 dim=2

   // Copy data from off-chip into BRAM locals
   for (int i = 0; i < HEIGHT; ++i) {
#pragma HLS pipeline II=1
       for (int j = 0; j < WIDTH; ++j) {
           A_local[i][j] = A[i][j];
           B_local[i][j] = B[i][j];
       }
   }

   // Compute using BRAM arrays
   for (int i = 0; i < HEIGHT; ++i) {

// Pipeline the loop
#pragma HLS pipeline II=1

       for (int j = 0; j < WIDTH; ++j) {
           int16_t D = (int16_t)A_local[i][j] - (int16_t)B_local[i][j];
           if (D < 0) D = -D;
           if (D < T1) {
               C_local[i][j] = 0;
           } else if (D < T2) {
               C_local[i][j] = 128;
           } else {
               C_local[i][j] = 255;
           }
       }
   }

   // Write back to off-chip
   for (int i = 0; i < HEIGHT; ++i) {
#pragma HLS pipeline II=1
       for (int j = 0; j < WIDTH; ++j) {
           C[i][j] = C_local[i][j];
       }
   }
}