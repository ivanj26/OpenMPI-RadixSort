#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

/**
 * Reference: https://www.geeksforgeeks.org/radix-sort/
*/

int getMax(int* arr, int n) {
	int max = arr[0];
	for (int i = 1; i < n; i++) {
		if (arr[i] > max) {
			max = arr[i];
		}
	}

	return max;
}

void countSort(int* arr, int n, int exp) {
    int* output = malloc(n * sizeof(int));
    int i, count[10] = {0}; 
   
    for (i = 0; i < n; i++) {
        count[ (arr[i]/exp)%10 ]++; 
    }

    for (i = 1; i < 10; i++) {
        count[i] += count[i - 1]; 
    }
  
    for (i = n - 1; i >= 0; i--) { 
        output[count[ (arr[i]/exp)%10 ] - 1] = arr[i]; 
        count[ (arr[i]/exp)%10 ]--; 
    }
  
    for (i = 0; i < n; i++) {
        arr[i] = output[i]; 
    }
}

void radixSort(int* arr, int n) {
    int max = getMax(arr, n); 
  
    for (int exp = 1; max/exp > 0; exp *= 10) {
    	countSort(arr, n, exp); 
    }
}

int main(int argc,char *argv[]) {
	int num_proc, rank;
	double start, stop;
	int n = atoi(argv[1]);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	int *arr = malloc(sizeof(int) * n);
	if (rank == 0) {
		rng(arr, n, 13516059);
	}

	start = MPI_Wtime();
	if (rank == 0) {
		radixSort(arr, n);
		output(arr, n);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	stop = MPI_Wtime();

	free(arr);
	if (rank == 0) {
		printf("Elapsed time for %d processor(s) = %lf ms\n", 1, (stop-start)/0.001);
	}
	
	MPI_Finalize();	
}