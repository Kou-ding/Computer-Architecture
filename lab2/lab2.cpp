#define HEIGHT 64
#define WIDTH 64
#define DATA_SIZE HEIGHT*WIDTH
#define BUFFER_SIZE 14
#define T1 32
#define T2 96

// TRIPCOUNT identifier
const unsigned int c_len = DATA_SIZE / BUFFER_SIZE;
const unsigned int c_size = BUFFER_SIZE;

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

extern "C" {
void imageDiffPosterize(const unsigned int *A,
			const unsigned int *B,
			unsigned int *C,
			unsigned int *C_filt,
			int size){

#pragma HLS INTERFACE m_axi port = A offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = B offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = C offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = C_filt offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = A bundle = control
#pragma HLS INTERFACE s_axilite port = B bundle = control
#pragma HLS INTERFACE s_axilite port = C bundle = control
#pragma HLS INTERFACE s_axilite port = C_filt bundle = control
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control


	// Local buffers that we want to implement in BRAM
	unsigned int A_buffer[BUFFER_SIZE];
	unsigned int B_buffer[BUFFER_SIZE];
	unsigned int C_buffer[BUFFER_SIZE];
	unsigned int C_filt_buffer[BUFFER_SIZE];

	// Filtering buffers
	int mid[BUFFER_SIZE + 2];
	int up[BUFFER_SIZE + 2];
	int down[BUFFER_SIZE + 2];

	// Difference
	long int D = 0;

  // Per iteration of this loop perform BUFFER_SIZE vector addition
  for (int i = 0; i < size; i += BUFFER_SIZE) {
#pragma HLS LOOP_TRIPCOUNT min = c_len max = c_len
    int chunk_size = BUFFER_SIZE;
    // boundary checks
    if ((i + BUFFER_SIZE) > size)
      chunk_size = size - i;


    read1:
	for (int j = 0; j < chunk_size; j++) {
 #pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
 #pragma HLS PIPELINE II = 1
	A_buffer[j] = A[i + j];
	}

    read2:
	for (int j = 0; j < chunk_size; j++) {
 #pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
 #pragma HLS PIPELINE II = 1
	B_buffer[j] = B[i + j];
	}

	imageDiff:
	for (int j = 0; j < chunk_size; j++) {
 #pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
 #pragma HLS PIPELINE II = 1
		// image difference
		D = A_buffer[j] - B_buffer[j];
		if(D<0) D=-D;
		if (D < T1) {
			C_buffer[j] = 0;
		} else if (D < T2) {
			C_buffer[j] = 128;
		} else {
			C_buffer[j] = 255;
		}
	}

    // burst write the result
    write:
        for (int j = 0; j < chunk_size; j++) {
     #pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
     #pragma HLS PIPELINE II = 1
        C[i + j] = C_buffer[j];
        }
  }
  for (int i = 0; i < size; i += BUFFER_SIZE) {
#pragma HLS LOOP_TRIPCOUNT min = c_len max = c_len
	  int chunk_size = BUFFER_SIZE;
	  // boundary checks
	  if ((i + BUFFER_SIZE) > size){
		  chunk_size = size - i;
	  }

	  read_3:
	  for (int j = -1; j <= chunk_size; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
#pragma HLS PIPELINE II = 1
		  int k = j + 1;           // safe buffer index in [0 .. chunk_size+1]
		  int idx = i + j;         // absolute index in flattened image

		  if (idx < 0 || idx >= size ) {
			  mid[k] = up[k] = down[k] = 0;


		  } else {
			  mid[k]  = C[idx];
			  up[k]   = (idx >= WIDTH)        ? C[idx - WIDTH] : 0;
			  down[k] = (idx + WIDTH < size)  ? C[idx + WIDTH] : 0;
		  }
	  }

	  filtering:
	  for (int j = 0; j < chunk_size; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
#pragma HLS PIPELINE II = 1
		  int idx = i + j; // absolute index of center pixel
		  int k   = j + 1; // center position in buffers

		  if (!is_interior(idx)) {
			  // Border output pixel: unchanged
			  C_filt_buffer[j] = clipper(mid[k]);
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
			  C_filt_buffer[j] = clipper(val);
		  }
	  }

	  write_filtering:
	  for (int j = 0; j < chunk_size; j++) {
#pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
#pragma HLS PIPELINE II = 1
		  C_filt[i+j] = C_filt_buffer[j];
	  }
  }

}
}
