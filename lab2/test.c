#include <stdio.h>

#define BUFFER_SIZE 3
#define WIDTH 5
#define HEIGHT 5

int main(){
    int C_buffer2[BUFFER_SIZE+2]={0};
    int C_buffer2up[BUFFER_SIZE+2]={0};
    int C_buffer2down[BUFFER_SIZE+2]={0};
    int size = HEIGHT*WIDTH;

    int C[25]={1,2,3,4,5,
        6,7,8,9,10,
        11,12,13,14,15,
        16,17,18,19,20,
        21,22,23,24,25};

    for (int i = 0; i < size; i += BUFFER_SIZE) {
        int chunk_size = BUFFER_SIZE;
        // boundary checks
        if ((i + BUFFER_SIZE) > size){
            chunk_size = size - i;
        }
        for (int j = 0; j < chunk_size+2; j++) {
            if(((i+j)>=WIDTH && (i+j)>=(HEIGHT*WIDTH-WIDTH) && (i+j)%WIDTH!=1 && (i+j)%WIDTH!=0) || ((j==0 || j==chunk_size+1) && (i+j)>=WIDTH && (i+j)>=(HEIGHT*WIDTH-WIDTH))){
                C_buffer2[j] = C[i + j-1];
                C_buffer2down[j] = C[i+j + WIDTH - 1];
                C_buffer2up[j] = C[i+j - WIDTH - 1];
            }
        }
        for(int j=0;j<chunk_size+2;j++){
            printf("%d ",C_buffer2up[j]);
        }
        printf("\n");
        for(int j=0;j<chunk_size+2;j++){
            printf("%d ",C_buffer2[j]);
        }
        printf("\n");
        for(int j=0;j<chunk_size+2;j++){
            printf("%d ",C_buffer2down[j]);
        }
        printf("\n-----------\n");
        for(int m=0;m<BUFFER_SIZE+2;m++){
            C_buffer2[m]=0;
            C_buffer2up[m]=0;
            C_buffer2down[m]=0;
        }
    }
    

    return 0;
}
