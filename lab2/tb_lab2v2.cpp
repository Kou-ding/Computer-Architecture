#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define HEIGHT 256
#define WIDTH 256
#define T1 32
#define T2 96

void imageDiffPosterize(const uint8_t A[HEIGHT][WIDTH],
                        const uint8_t B[HEIGHT][WIDTH],
                        uint8_t C[HEIGHT][WIDTH]);

int main(){
    srand(3);  // Fixed seed for reproducibility
    unsigned char A[HEIGHT][WIDTH], B[HEIGHT][WIDTH], C[HEIGHT][WIDTH], C_golden[HEIGHT][WIDTH];

    // Initialize input data
    for(int i = 0; i < HEIGHT; i++){
        for(int j = 0; j < WIDTH; j++){
            A[i][j] = rand() % 256;
            B[i][j] = rand() % 256;
        }
    }

    // Golden reference computation
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            int16_t D = (int16_t)A[i][j] - (int16_t)B[i][j];
            if (D < 0) D = -D;
            if (D < T1) {
                C_golden[i][j] = 0;
            } else if (D < T2) {
                C_golden[i][j] = 128;
            } else {
                C_golden[i][j] = 255;
            }
        }
    }

    // Call hardware function
    imageDiffPosterize(A, B, C);

    // Verification loop
    int errors = 0;
    for (int i = 0; i < HEIGHT; i++){
        for (int j = 0; j < WIDTH; j++){
            if (C[i][j] != C_golden[i][j]){
                printf("Mismatch at (%d,%d): HW = %d, SW = %d", i, j, C[i][j], C_golden[i][j]);
                errors++;
            }
        }
    }

    // Print sample output (8×8 corner)
    printf("HW output (8×8 sample):");
    for (int i = 0; i < 8; i++){
        for (int j = 0; j < 8; j++){
            printf(" %3d", C[i][j]);
        }
        printf("");
    }

    printf("Golden output (8×8 sample):");
    for (int i = 0; i < 8; i++){
        for (int j = 0; j < 8; j++){
            printf(" %3d", C_golden[i][j]);
        }
        printf("");
    }

    // Report results
    if (errors == 0) {
        printf("Test PASSED");
        return 0;  // Return 0 for success
    } else {
        printf("Test FAILED with %d errors", errors);
        return 1;  // Return non-zero for failure
    }
}