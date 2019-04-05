#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void rng(int* arr, int n, int seed) {
    srand(seed);
    for(long i = 0; i < n; i++) {
        arr[i] = (int)rand();
    }
}

void print_array(int *arr, int n){
    for (int i = 0; i < n; i++) {
        printf("%d\n", arr[i]);
    }
    printf("\n");
}

void countDigit(int* arr, int* count, int arrSize, int base) {
	for (int i = 0; i < arrSize; i++) {
		int bit = (arr[i] & base) ? 1 : 0;
		count[bit]++;
	}
}

void calculateOffset(int* count, int* offset, int offsetSize) {
	offset[0] = 0;
	for (int i = 1; i < offsetSize; i++) {
		offset[i] = offset[i-1] + count[i-1];
	}
}

void radixSort(int* arr, int* count, int arrSize, int* offset, int base) {
	int* output = malloc(arrSize * sizeof(int));
	for (int i = 0; i < arrSize; i++) {
		int bit = (arr[i] & base) ? 1 : 0;
		output[offset[bit]++] = arr[i];
	}

	memcpy(arr, output, arrSize * sizeof(int));
	free(output);
}

int main(int argc, char *argv[]) {
	assert(argc > 1);

	int n = atoi(argv[1]);

	int num_proc, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int *arr = malloc(sizeof(int) * n);
	if (rank == 0) {
		// arr[0] = 40;
		// arr[1] = 22;
		// arr[2] = 11;
		// arr[3] = 823;
		assert(arr != NULL);

		rng(arr, n, 13516104);

		printf("All elements: \n");
		print_array(arr, n);	
	}

	int chunkSize = n / num_proc;
	int* chunkArr = (int*) malloc(sizeof(int) * chunkSize);

	MPI_Scatter(arr, chunkSize, MPI_INT, chunkArr, chunkSize, MPI_INT, 0, MPI_COMM_WORLD);
	
	printf("MyRank %d\n", rank);
	printf("Before:\n");
	print_array(chunkArr, chunkSize);

	for (int i = 0; i < 32; i++) {
		int countLocal[2] = {0};
		countDigit(chunkArr, countLocal, chunkSize, 1 << i);
		// print_array(countLocal, 2);

		int offsetLocal[2] = {0};
		calculateOffset(countLocal, offsetLocal, 2);
		// print_array(offsetLocal, 2);

		radixSort(chunkArr, countLocal, chunkSize, offsetLocal, 1 << i);
	}

	printf("After:\n");
	print_array(chunkArr, chunkSize);

	// for (int i = 0; i < num_proc; i++) {
	// 	// printf("> %d", countAll[i]);
	// }

	// free(chunkArr);
	MPI_Finalize();
}
