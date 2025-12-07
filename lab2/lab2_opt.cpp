#include <stdint.h>

#define HEIGHT 256
#define WIDTH 256
#define T1 32
#define T2 96

void imageDiffPosterize(const uint8_t A[HEIGHT][WIDTH],
		const uint8_t B[HEIGHT][WIDTH],
		uint8_t C[HEIGHT][WIDTH]){

	// Initialize D
	int16_t D;

// Configure memory interfaces and optimize data access patterns
// INTERFACE pragmas: Use ap_memory mode for efficient RAM block access
#pragma HLS INTERFACE mode=ap_memory port=A
#pragma HLS INTERFACE mode=ap_memory port=B
#pragma HLS INTERFACE mode=ap_memory port=C

// Array partitioning: Completely partition dim=2 to create separate ports for each column,
// enabling parallel reads/writes across the matrix width and eliminating memory access bottlenecks
#pragma HLS array_partition variable=A complete dim=2
#pragma HLS array_partition variable=B complete dim=2
#pragma HLS array_partition variable=C complete dim=2


	for(int i=0;i<HEIGHT;i++){

// Pipeline the loop
#pragma HLS pipeline II=1

		for(int j=0;j<WIDTH;j++){
			// Unroll the loop
			//#pragma HLS unroll factor=2

			// Calculate the absolute of the difference
			D=(int16_t)A[i][j]-(int16_t)B[i][j];
			if(D<0){
				D=-D;
			}

			// Generate C matrix
			if(D<T1){
				C[i][j]=0;
			} else if(D<T2){
				C[i][j]=128;
			} else {
				C[i][j]=255;
			}
		}
	}
}