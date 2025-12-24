/*******************************************************************************
Description:
    Wide Memory Access Example using ap_uint<Width> datatype
    Description: This is vector addition example to demonstrate Wide Memory
    access of 512bit Datawidth using ap_uint<> datatype which is defined inside
    'ap_int.h' file.
*******************************************************************************/

//Including to use ap_uint<> datatype
#include <ap_int.h>
#include <stdio.h>
#include <string.h>

#define T1 32
#define T2 96

#define BUFFER_SIZE 64
#define DATAWIDTH 512
#define VECTOR_SIZE (DATAWIDTH / 32) // vector size is 16 (512/32 = 16)
typedef ap_uint<DATAWIDTH> uint512_dt;

// TRIPCOUNT identifier
const unsigned int c_chunk_sz = BUFFER_SIZE;
const unsigned int c_size     = VECTOR_SIZE;

int clipper(int element){
    if (element>255){
        element=255;
    }
    else if (element<0){
        element=0;
    }
    return element;
}
int is_interior(int idx) {
    int row = idx / WIDTH;
    int col = idx % WIDTH;
    return (row > 0 && row < HEIGHT-1 && col > 0 && col < WIDTH-1);
}

/*
    Vector Addition Kernel Implementation using uint512_dt datatype
    Arguments:
        in1   (input)     --> Input Vector1
        in2   (input)     --> Input Vector2
        out   (output)    --> Output Vector
        size  (input)     --> Size of Vector in Integer
   */
extern "C"
{
    void imageDiffPosterize(
        const uint512_dt *A, // Read-Only Vector 1
        const uint512_dt *B, // Read-Only Vector 2
        uint512_dt *C,       // Intermediate Output Result
        uint521_dt *C_filt,  // Output Result
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
        uint512_dt C_local[BUFFER_SIZE]; // Local Memory to store results
        uint512_dt C_filt_local[BUFFER_SIZE];

        // Filtering buffers
        int mid[BUFFER_SIZE + 2];
        int up[BUFFER_SIZE + 2];
        int down[BUFFER_SIZE + 2];

        // Input vector size for integer vectors. However kernel is directly
        // accessing 512bit data (total 16 elements). So total number of read
        // from global memory is calculated here:
        // Eg. size = 16 ints -> ((16-1)/16)+1=1 -> 1 memory access needed
        // Eg. size = 17 ints -> ((17-1)/16)+1=2 -> 2 memory access needed
        int size_in16 = (size - 1) / VECTOR_SIZE + 1;

        //Per iteration of this loop perform BUFFER_SIZE vector addition
        for (int i = 0; i < size_in16; i += BUFFER_SIZE) {
//#pragma HLS PIPELINE
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
                uint512_dt tmpA = A_local[j];
                uint512_dt tmpB = B_local[j];

                uint512_dt tmpC = 0;

                for (int vector = 0; vector < VECTOR_SIZE; vector++) {
#pragma HLS UNROLL
                    // ap_uint<32> tmp1 = tmpV1.range(32 * (vector + 1) - 1, vector * 32);
                    // ap_uint<32> tmp2 = tmpV2.range(32 * (vector + 1) - 1, vector * 32);
                    // tmpC.range(32 * (vector + 1) - 1, vector * 32) = tmp1 + tmp2;
                    //out[i + j] = tmpV1 + tmpV2; // Vector Addition Operation

                    // image difference
                    ap_uint<32> D = tmp1 - tmp2;
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
        
        // Begin loop anew after forming C matrix    
        for (int i = 0; i < size_in16; i += BUFFER_SIZE) {
//#pragma HLS PIPELINE
#pragma HLS DATAFLOW
#pragma HLS stream variable = A_local depth = 64
#pragma HLS stream variable = B_local depth = 64

            int chunk_size = BUFFER_SIZE;

            //boundary checks
            if ((i + BUFFER_SIZE) > size_in16)
                chunk_size = size_in16 - i;

            C_read:
            for (int j = -1; j <= chunk_size; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
#pragma HLS PIPELINE II = 1
                int k = j + 1;           // safe buffer index in [0 .. chunk_size+1]
                int idx = i + j;         // absolute index in flattened image

                for (int vector = 0; vector < VECTOR_SIZE; vector++) {
#pragma HLS UNROLL

                    if (idx < 0 || idx >= size ) {
                        mid[k] = up[k] = down[k] = 0;
                    } else {
                        mid[k]  = tmpC.range(32 * (vector + 1) - 1, vector * 32);
                        up[k]   = (idx >= WIDTH)        ? C[idx - WIDTH] : 0;
                        down[k] = (idx + WIDTH < size)  ? C[idx + WIDTH] : 0;
                    }
                }
            }
            filtering:
            for (int j = 0; j < chunk_size; j++) {
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 64

                int idx = i + j; // absolute index of center pixel
                int k   = j + 1; // center position in buffers

                uint512_dt tmpC_filt = 0;
                for (int vector = 0; vector < VECTOR_SIZE; vector++) {
#pragma HLS UNROLL
                    if (!is_interior(idx)) {
                        // Border output pixel: unchanged
                        C_filt_local[j] = clipper(mid[k]);
                    } else {
                        // Interior output pixel: full 3x3 available from buffers, including border values.
                        //
                        // 3x3 window:
                        // up:   up[k-1]   up[k]   up[k+1]
                        // mid:  mid[k-1]  mid[k]  mid[k+1]
                        // down: down[k-1] down[k] down[k+1]
                        //
                        // Sharpen kernel (as in your lab):
                        //  0 -1  0
                        // -1  5 -1
                        //  0  -1 0
                        int center = mid[k];
                        int up_c   = up[k];
                        int down_c = down[k];
                        int left_c = mid[k - 1];
                        int right_c= mid[k + 1];

                        int val = 5 * center - up_c - down_c - left_c - right_c;
                        C_filt_local[j] = clipper(val);
                    }
                    tmpC_filt.range(32 * (vector + 1) - 1, vector * 32)[i+j] = C_filt_local[j];
                }
                
            }

        }
    }
}
