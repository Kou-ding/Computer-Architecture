#include <stdio.h>
#include <stdlib.h>

#define HEIGHT 64
#define WIDTH 64
#define DATA_SIZE HEIGHT*WIDTH
#define BUFFER_SIZE 14
#define T1 32
#define T2 96

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

void imageDiffPosterize(const unsigned int *A,
                        const unsigned int *B,
                        unsigned int *C,
                        unsigned int *C_filt,
                        int size) {

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

    for (int i = 0; i < size; i += BUFFER_SIZE) {
// #pragma HLS LOOP_TRIPCOUNT min = c_len max = c_len
        int chunk_size = BUFFER_SIZE;
        // boundary checks
        if ((i + BUFFER_SIZE) > size){
            chunk_size = size - i;
        }


        read1:
        for (int j = 0; j < chunk_size; j++) {
    // #pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
    // #pragma HLS PIPELINE II = 1
            A_buffer[j] = A[i + j];
        }

        read2:
        for (int j = 0; j < chunk_size; j++) {
    // #pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
    // #pragma HLS PIPELINE II = 1
            B_buffer[j] = B[i + j];
        }

        imageDiff:
        for (int j = 0; j < chunk_size; j++) {
    // #pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
    // #pragma HLS PIPELINE II = 1
            // perform vector addition
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
        write:
        for (int j = 0; j < chunk_size; j++) {
    // #pragma HLS LOOP_TRIPCOUNT min = c_size max = c_size
    // #pragma HLS PIPELINE II = 1  
            C[i + j] = C_buffer[j];
        }

        
    }
    for (int i = 0; i < size; i += BUFFER_SIZE) {
// #pragma HLS LOOP_TRIPCOUNT min = c_len max = c_len
        int chunk_size = BUFFER_SIZE;
        // boundary checks
        if ((i + BUFFER_SIZE) > size){
            chunk_size = size - i;
        }
        read_3:
        for (int j = -1; j <= chunk_size; j++) {
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
            C_filt[i+j] = C_filt_buffer[j];
        }
    }
}

void sw_ref(unsigned int *A, unsigned int *B, unsigned int *C_SW, unsigned int *C_SW_filt){

    unsigned long int D;
    unsigned int S[(WIDTH-2)*(HEIGHT-2)]={0};
    int F[3][3] = {0, -1, 0, -1, 5, -1, 0, -1, 0};

    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            D = A[i*WIDTH + j] - B[i*WIDTH + j];
            if(D<0) D=-D;
            if(D<T1){
                C_SW[i*WIDTH + j]=0;
            } else if(D<T2){
                C_SW[i*WIDTH + j]=128;
            } else {
                C_SW[i*WIDTH + j]=255;
            }
        }
    }
    
    for (int i = 1; i <= HEIGHT - 2; i++) {
        for (int j = 1; j <= WIDTH - 2; j++) {
            for (int a = -1; a <= 1; a++) {
                for (int b = -1; b <= 1; b++) {
                    S[(i - 1)*(WIDTH-2)+(j - 1)] += F[a + 1][b + 1] * C_SW[(i + a)*WIDTH+(j + b)];
                }
            }
        }
    }  

    // Clipping
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if(i>=1 && i<=HEIGHT-2 && j>=1 && j<=WIDTH-2){
                C_SW_filt[i*WIDTH+j]=clipper(S[(i-1)*(WIDTH-2)+(j-1)]);
            }else{
                C_SW_filt[i*WIDTH+j]=clipper(C_SW[i*WIDTH+j]);
            }
        }
    }
}

int main(){
    // Initialize variables
    int errors=0;
    unsigned int *A = (unsigned int*)malloc(DATA_SIZE * sizeof(unsigned int));
    unsigned int *B = (unsigned int*)malloc(DATA_SIZE * sizeof(unsigned int));
    unsigned int *C = (unsigned int*)malloc(DATA_SIZE * sizeof(unsigned int));
    unsigned int *C_SW = (unsigned int*)malloc(DATA_SIZE * sizeof(unsigned int));
    unsigned int *C_SW_filt = (unsigned int*)malloc(DATA_SIZE * sizeof(unsigned int));
    unsigned int *C_filt = (unsigned int*)malloc(DATA_SIZE * sizeof(unsigned int));
    int size=DATA_SIZE;

    // Check for allocation failures
    if (!A || !B || !C || !C_SW || !C_SW_filt) {
        printf("Memory allocation failed!\n");
        return -1;
    }

    srand(3);
    // Populate A and B
    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            A[i*WIDTH+j] = rand() % 256;
            B[i*WIDTH+j] = rand() % 256;
        }
    }

    // Run functions
    imageDiffPosterize(A, B, C, C_filt, size);
    sw_ref(A, B, C_SW, C_SW_filt);

    // Calculate SW and HW differences
    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            if(C_SW_filt[i*WIDTH+j]!=C_filt[i*WIDTH+j]){
                errors++;
            }
        }
    }

    // Report
    if(errors>=1){
        printf("There are %d different pixels.\n", errors);
    }
    else
    {
        printf("Test passed!\n");
    }

    // Free allocated memory
    free(A);
    free(B);
    free(C);
    free(C_SW);
    free(C_filt);
    free(C_SW_filt);
    
    return 0;
}