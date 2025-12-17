#include <stdio.h>

#define HEIGHT 5
#define WIDTH 5

int clipper(int element){
  if (element>255){
    element=255;
  }
  else if (element<0){
    element=0;
  }
  return element;
}

int main() {

  int C[HEIGHT][WIDTH];
  int C_filt[HEIGHT][WIDTH];
  int S[HEIGHT-2][WIDTH-2] = {0};

  for (int i = 0; i < HEIGHT; i++) {
    for (int j = 0; j < WIDTH; j++) {
      C[i][j] = 1;
    }
  }
  C[3][3] = 1000;

  // Filter
  int F[3][3] = {0, -1, 0, -1, 5, -1, 0, -1, 0};

  for (int i = 1; i <= HEIGHT - 2; i++) {
    for (int j = 1; j <= WIDTH - 2; j++) {
      for (int a = -1; a <= 1; a++) {
        for (int b = -1; b <= 1; b++) {
          S[i - 1][j - 1] = S[i - 1][j - 1] + F[a + 1][b + 1] * C[i + a][j + b];
        }
      }
    }
  }

  // Clipping
  for (int i = 0; i < HEIGHT; i++) {
    for (int j = 0; j < WIDTH; j++) {
      if(i>=1 && i<=HEIGHT-2 && j>=1 && j<=WIDTH-2){
        C_filt[i][j]=clipper(S[i-1][j-1]);
      }else{
        C_filt[i][j]=clipper(C[i][j]);
      }
    }
  }
  // Print S Matrix
  for (int i = 0; i < HEIGHT-2; i++) {
    for (int j = 0; j < WIDTH-2; j++) {
      printf("%d ", S[i][j]);
    }
    printf("\n");
  }

  printf("\n-----------\n\n");
  // Print C matrix
  for (int i = 0; i < HEIGHT; i++) {
    for (int j = 0; j < WIDTH; j++) {
      printf("%d ", C_filt[i][j]);
    }
    printf("\n");
  }

  return 0;
}
