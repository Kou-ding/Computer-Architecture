#include <stdio.h>
#include <stdint.h>

#define HEIGHT 256
#define WIDTH 256
#define T1 32
#define T2 96


void imageDiffPosterize(uint8_t A[HEIGHT][WIDTH], uint8_t B[HEIGHT][WIDTH], uint8_t C[HEIGHT][WIDTH]);


void sw_ref(uint8_t A[HEIGHT][WIDTH], uint8_t B[HEIGHT][WIDTH], uint8_t C_SW[HEIGHT][WIDTH]){
    int16_t D[HEIGHT][WIDTH];
    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            D[i][j]=(int16_t)A[i][j]-(int16_t)B[i][j];
            if(D[i][j]<0)D[i][j]=-D[i][j];
            if(D[i][j]<T1){
                C_SW[i][j]=0;
            } else if(D[i][j]<T2){
                C_SW[i][j]=128;
            } else {
                C_SW[i][j]=255;
            }
        }
    }
}

int main(){

    // Initialize variables
    int errors=0;
    uint8_t A[HEIGHT][WIDTH];
    uint8_t B[HEIGHT][WIDTH];
    uint8_t C[HEIGHT][WIDTH];
    uint8_t C_SW[HEIGHT][WIDTH];

    // Populate A and B
    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            A[i][j]=(uint8_t)i;
            B[i][j]=(uint8_t)j;
        }
    }

    // Run functions
    imageDiffPosterize(A, B, C);
    sw_ref(A, B, C_SW);

    // Calculate SW and HW differences
    for(int i=0;i<HEIGHT;i++){
        for(int j=0;j<WIDTH;j++){
            if(C_SW[i][j]!=C[i][j]){
                errors+=1;
            }
        }
    }

    // Report
    if(errors>=1){
        printf("There are %d different pixels.", errors);
    }
    else
    {
        printf("Test passed!");
    }
    return 0;
}