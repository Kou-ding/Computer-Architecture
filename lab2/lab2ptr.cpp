#include <stdio.h>
#include <stdlib.h>

#define HEIGHT 256
#define WIDTH 256
#define DATA_SIZE HEIGHT*WIDTH
#define BUFFER_SIZE 256 // Must be same as width
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

void imageDiffPosterize(const unsigned int *A,
                        const unsigned int *B,
                        unsigned int *C,
                        int size) {

   // Local buffers that we want to implement in BRAM
   unsigned int A_buffer[BUFFER_SIZE];
   unsigned int B_buffer[BUFFER_SIZE];
   unsigned int C_buffer[BUFFER_SIZE];
   unsigned int C_filt[DATA_SIZE];
   unsigned int S_buffer[BUFFER_SIZE]={0};


    // Filter
    int F[3][3] = {0, -1, 0, -1, 5, -1, 0, -1, 0};

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

        vadd:
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

    int Neighbors[3][3];
    int S=0;
    read_C_Neighbors:
    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            if(!(i==0 || j==0 || i==HEIGHT-1 || j==WIDTH-1)){
                for (int k=-1;k<=1;k++){
                    for (int l=-1;l<=1;l++){
                        Neighbors[k+1][l+1]=C[(i+k)*WIDTH+(j+l)];
                    }
                }
                S=0;
                for (int a = -1; a <= 1; a++) {
                    for (int b = -1; b <= 1; b++) {
                        // Filtering
                        S += F[a + 1][b + 1] * Neighbors[a+1][b+1];
                    }
                }
                C_filt[i*WIDTH+j]=clipper(S);
            }else{
                C_filt[i*WIDTH+j]=clipper(C[i*WIDTH+j]);
            }
        }    
    }
    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            C[i*WIDTH+j]=C_filt[i*WIDTH+j];
        }
    }


}

void sw_ref(unsigned int *A, unsigned int *B, unsigned int *C_SW){

    unsigned long int D;
    unsigned int S[(WIDTH-2)*(HEIGHT-2)]={0};
    int F[3][3] = {0, -1, 0, -1, 5, -1, 0, -1, 0};
    int C_filt[WIDTH*HEIGHT];

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
                    S[(i - 1)*(WIDTH-2)+(j - 1)] = S[(i - 1)*(WIDTH-2)+(j - 1)] + F[a + 1][b + 1] * C_SW[(i + a)*WIDTH+(j + b)];
                }
            }
        }
    }  

    // Clipping
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if(i>=1 && i<=HEIGHT-2 && j>=1 && j<=WIDTH-2){
                C_SW[i*WIDTH+j]=clipper(S[(i-1)*(WIDTH-2)+(j-1)]);
            }else{
                C_SW[i*WIDTH+j]=clipper(C_SW[i*WIDTH+j]);
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
    int size=DATA_SIZE;

    // Check for allocation failures
    if (!A || !B || !C || !C_SW) {
        printf("Memory allocation failed!\n");
        return -1;
    }

    // Populate A and B
    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            A[i*WIDTH+j]=(unsigned int)i;
            B[i*WIDTH+j]=(unsigned int)j;
        }
    }

    // Run functions
    imageDiffPosterize(A, B, C, size);
    sw_ref(A, B, C_SW);

    // Calculate SW and HW differences
    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            if(C_SW[i*WIDTH+j]!=C[i*WIDTH+j]){
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
        printf("Test passed!");
    }

    // Free allocated memory
    free(A);
    free(B);
    free(C);
    free(C_SW);
    
    return 0;
}