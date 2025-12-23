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

        uint512_dt v1_local[BUFFER_SIZE]; // Local memory to store vector1
        uint512_dt v2_local[BUFFER_SIZE];
        uint512_dt result_local[BUFFER_SIZE]; // Local Memory to store result

        // Input vector size for integer vectors. However kernel is directly
        // accessing 512bit data (total 16 elements). So total number of read
        // from global memory is calculated here:
        int size_in16 = (size - 1) / VECTOR_SIZE + 1;

        //Per iteration of this loop perform BUFFER_SIZE vector addition
        for (int i = 0; i < size_in16; i += BUFFER_SIZE) {
//#pragma HLS PIPELINE
#pragma HLS DATAFLOW
#pragma HLS stream variable = v1_local depth = 64
#pragma HLS stream variable = v2_local depth = 64

            int chunk_size = BUFFER_SIZE;

            //boundary checks
            if ((i + BUFFER_SIZE) > size_in16)
                chunk_size = size_in16 - i;

        //burst read first vector from global memory to local memory
        v1_rd:
            for (int j = 0; j < chunk_size; j++) {
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 64
                v1_local[j] = in1[i + j];
                v2_local[j] = in2[i + j];
            }

        //burst read second vector and perform vector addition
        add:
			for (int j = 0; j < chunk_size; j++) {
			#pragma HLS pipeline
			#pragma HLS LOOP_TRIPCOUNT min = 1 max = 64
							uint512_dt tmpV1 = v1_local[j];
							uint512_dt tmpV2 = v2_local[j];

							uint512_dt tmpOut = 0;

							for (int vector = 0; vector < VECTOR_SIZE; vector++) {
							   #pragma HLS UNROLL
								ap_uint<32> tmp1 = tmpV1.range(32 * (vector + 1) - 1, vector * 32);
								ap_uint<32> tmp2 = tmpV2.range(32 * (vector + 1) - 1, vector * 32);
								tmpOut.range(32 * (vector + 1) - 1, vector * 32) = tmp1 + tmp2;
							}

							out[i + j] = tmpOut;

							//out[i + j]       = tmpV1 + tmpV2; // Vector Addition Operation
			}
        }
    }
}
