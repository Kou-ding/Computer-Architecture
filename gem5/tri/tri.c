// Function that computes the Triangular number T_N
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){

    int N = atoi(argv[1]);

    if (argc!=2||N<0){
        printf("Wrong Function Usage.\nFormat: %s N \n(where N any positive natural number)\n",argv[0]);
        return -1;
    }
    long long int triangular =(N*(N+1)) / 2;

    printf("T_%d: %lld\n",N,triangular);
    return 0;
}