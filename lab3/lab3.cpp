//Including to use ap_uint<> datatype
#include <ap_int.h>
#include <stdio.h>
#include <string.h>

#define T1 32
#define T2 96

// Real matrix dimensions
#define HEIGHT 128
#define WIDTH 128

#define BUFFER_SIZE 64 // number of 512bit integers in one buffer
#define DATAWIDTH 512 // bits in one 512bit integer
#define VECTOR_SIZE (DATAWIDTH / 32) // number of 32bit integers in one 512bit (512/32 = 16)
typedef ap_uint<DATAWIDTH> uint512_dt;

// TRIPCOUNT identifier
const unsigned int c_chunk_sz = BUFFER_SIZE;
const unsigned int c_size     = VECTOR_SIZE;

ap_uint<32> clipper(ap_int<64> element){
    if (element>255){
        element=255;
    }
    else if (element<0){
        element=0;
    }
    return (ap_int<32>)element;
}
// Refactor it to work with the vectorized code
int is_interior(int idx) {
    int row = idx / WIDTH;
    int col = idx % WIDTH;
    return (row > 0 && row < HEIGHT-1 && col > 0 && col < WIDTH-1);
}

ap_int<64> sharpen(ap_uint<32> mid, ap_uint<32> right, ap_uint<32> left, ap_uint<32> up, ap_uint<32> down){
    ap_int<64> sharpPixel = 5 * mid - up - down - left - right;
    return sharpPixel;
}

extern "C"
{
    void imageDiffPosterize(
        const uint512_dt *A, // Read-Only Vector 1
        const uint512_dt *B, // Read-Only Vector 2
        uint512_dt *C,       // Intermediate Output Result
        uint512_dt *C_filt,  // Output Result
        int size             // Size in integer
    )
    {
#pragma HLS INTERFACE m_axi port = A bundle = gmem
#pragma HLS INTERFACE m_axi port = B bundle = gmem1
#pragma HLS INTERFACE m_axi port = C bundle = gmem2
#pragma HLS INTERFACE m_axi port = C_filt bundle = gmem3
#pragma HLS INTERFACE s_axilite port = A bundle = control
#pragma HLS INTERFACE s_axilite port = B bundle = control
#pragma HLS INTERFACE s_axilite port = C bundle = control
#pragma HLS INTERFACE s_axilite port = C_filt bundle = control
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

        uint512_dt A_local[BUFFER_SIZE]; // Local memory to store input vectors
        uint512_dt B_local[BUFFER_SIZE];
        uint512_dt C_local[BUFFER_SIZE+((WIDTH/VECTOR_SIZE)*2)]; // Local Memory to store an extended C buffer

        // Input vector size for integer vectors. However kernel is directly
        // accessing 512bit data (total 16 elements). So total number of read
        // from global memory is calculated here:
        // Eg. size = 16 integers -> ((16-1)/16)+1=1 -> 1 memory access needed
        // Eg. size = 17 integers -> ((17-1)/16)+1=2 -> 2 memory access needed
        int size_in16 = (size - 1) / VECTOR_SIZE + 1;

        // Deal with 64x512bit buffers at a time 
        // (the last chunk might effectively create a smaller buffer)
        // (the buffer will still have 64 locations but j will be limited to how many 512 numbers are left)
        for (int i = 0; i < size_in16; i += BUFFER_SIZE) {
// #pragma HLS PIPELINE
#pragma HLS DATAFLOW
#pragma HLS stream variable = A_local depth = 64
#pragma HLS stream variable = B_local depth = 64

            int chunk_size = BUFFER_SIZE;

            //boundary checks
            if ((i + BUFFER_SIZE) > size_in16)
                chunk_size = size_in16 - i;

        //burst read first vector from global memory to local memory
        A_B_read:
            for (int j = 0; j < chunk_size; j++) {
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 64
                A_local[j] = A[i + j];
                B_local[j] = B[i + j];
            }

        //burst read second vector and perform vector addition
        imageDiff:
			for (int j = 0; j < chunk_size; j++) {
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 64
                uint512_dt tmpA_512 = A_local[j];
                uint512_dt tmpB_512 = B_local[j];

                uint512_dt tmpC = 0;

                for (int vector = 0; vector < VECTOR_SIZE; vector++) {
#pragma HLS UNROLL
                    ap_uint<32> tmpA_32 = tmpA_512.range(32 * (vector + 1) - 1,vector * 32);
                    ap_uint<32> tmpB_32 = tmpB_512.range(32 * (vector + 1) - 1,vector * 32);
                    // image difference
                    ap_int<64> D = tmpA_32 - tmpB_32;
                    if(D<0) D=-D;
                    if (D < T1) {
                        tmpC.range(32 * (vector + 1) - 1, vector * 32) = 0;
                    } else if (D < T2) {
                        tmpC.range(32 * (vector + 1) - 1, vector * 32) = 128;
                    } else {
                        tmpC.range(32 * (vector + 1) - 1, vector * 32) = 255;
                    }
                }
                C[i + j] = tmpC;
			}
        }
        // Begin loop anew after forming C matrix    
        for (int i = 0; i < size_in16; i += BUFFER_SIZE) {
#pragma HLS DATAFLOW
            int chunk_size = BUFFER_SIZE;

            //boundary checks
            if ((i + BUFFER_SIZE) > size_in16)
                chunk_size = size_in16 - i;

            C_read:
            // Each C_local buffer has 8 real rows of 32bit integers
            // plus (WIDTH/VECTOR_SIZE) 512bit elements in the front and back of the buffer
            for (int j = 0; j < chunk_size+((WIDTH/VECTOR_SIZE)*2); j++) {
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 64
                // check if I am within the first and last 512bit element of the C matrix
                if(i+j-(WIDTH/VECTOR_SIZE)>=0 && i+j-(WIDTH/VECTOR_SIZE)<=size_in16){
                    C_local[j] = C[i + j - (WIDTH/VECTOR_SIZE)]; // Load one 512bit from the right and left edge
                }else{
                    C_local[j] = 0; // Handle elements outside C matrix
                }
            }

            // Traverse the normal chunksize
            for (int j = 0; j < chunk_size; j++) {
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 64
                // The first and last (WIDTH/VECTOR_SIZE) elements are for the computation of the main chunk_size elements
                // Their up down left and right that might be missing from the main chunk
                int real_j=j+(WIDTH/VECTOR_SIZE);

                // Local C temporary variables
                uint512_dt C_local_mid = C_local[real_j];
                uint512_dt C_local_left = C_local[real_j-1];
                uint512_dt C_local_right = C_local[real_j+1];
                uint512_dt C_local_up = C_local[real_j-(WIDTH/VECTOR_SIZE)];
                uint512_dt C_local_down = C_local[real_j+(WIDTH/VECTOR_SIZE)];

                uint512_dt tmpC_filt = 0;

                for (int vector = 0; vector < VECTOR_SIZE; vector++) {
#pragma HLS UNROLL
                    // 512bit integer index on the i,j matrix BUFFERSIZE*(number of buffers that need to be created to reach the total number of integers inside the HEIGHT*WIDTH matrix)
                    int idx = i + j;
                    // 32bit integer index on the HEIGHT*WIDTH matrix
                    int ichi = (idx)*VECTOR_SIZE + vector; 

                    // Temporary variables for mid, up, down, left, right
                    ap_uint<32> tmpMid = 0;
                    ap_uint<32> tmpUp = 0;
                    ap_uint<32> tmpDown = 0;
                    ap_uint<32> tmpLeft = 0;
                    ap_uint<32> tmpRight = 0;

                    // Verify that it is an inside element
                    if(is_interior(ichi)){
                        tmpMid = C_local_mid.range(32 * (vector + 1) - 1, vector * 32);

                        tmpUp = C_local_up.range(32 * (vector + 1) - 1, vector * 32);

                        tmpDown = C_local_down.range(32 * (vector + 1) - 1, vector * 32);

                        // Go to the previous 512bit to find the left element
                        if(vector==0){
                            tmpLeft = C_local_left.range(511, 480);
                            tmpRight = C_local_mid.range(32 * ((vector+1)+1) - 1, (vector+1) * 32);
                        // Go to the next 512bit to find the left element
                        }else if(vector==VECTOR_SIZE-1){
                            tmpRight = C_local_right.range(31, 0);
                            tmpLeft = C_local_mid.range(32 * ((vector-1)+1) - 1, (vector-1) * 32);
                        // Remain within your 512bit element
                        }else{
                            tmpRight = C_local_mid.range(32 * ((vector+1)+1) - 1, (vector+1) * 32);
                            tmpLeft = C_local_mid.range(32 * ((vector-1)+1) - 1, (vector-1) * 32);
                        }
                        ap_int<64> sharpPixel = sharpen(tmpMid, tmpRight, tmpLeft, tmpUp, tmpDown);
                        ap_uint<32> clipped_sharpPixel = clipper(sharpPixel);
                        tmpC_filt.range(32 * (vector + 1) - 1, vector * 32) = clipped_sharpPixel; 
                    }else{
                        tmpMid = C_local[real_j].range(32 * (vector + 1) - 1, vector * 32);
                        ap_uint<32> clipped_sharpPixel = clipper(tmpMid);
                        tmpC_filt.range(32 * (vector + 1) - 1, vector * 32) = clipped_sharpPixel;
                    }                       
                }
                C_filt[i+j] = tmpC_filt;
            }
        }
    }
}
