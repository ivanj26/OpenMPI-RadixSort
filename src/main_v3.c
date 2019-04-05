#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"

void countDigit(int* arr, int* count, int arrSize, int base) {
	for (int i = 0; i < arrSize; i++) {
		int bit = (arr[i] & base) ? 1 : 0;
		count[bit]++;
	}
}

void calculateOffset(int* count, int num_proc, int* offset, int offsetSize) {
	int cnt[2 * num_proc];
	memset(cnt, 0, 2 * num_proc * sizeof(int));
	int idx = 0;
	for(int i = 0; i < 2 * num_proc; i += 2) {
		cnt[idx] = count[i];
		idx++;
	}
	for(int i = 1; i < 2 * num_proc; i += 2) {
		cnt[idx] = count[i];
		idx++;
	}

	offset[0] = 0;
	for (int i = 1; i < offsetSize; i++) {
		offset[i] = offset[i-1] + cnt[i-1];
	}
}

void calculateOffsetOld(int* count, int* offset, int offsetSize) {
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
	double start, stop;
	assert(argc > 1);

	int n = atoi(argv[1]);

	int num_proc, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int *arr = malloc(sizeof(int) * n);
	if (rank == 0) {
		assert(arr != NULL);
		rng(arr, n, 13516059);
	}

	int chunkSize = n / num_proc;
	int* chunkArr = (int*) malloc(sizeof(int) * chunkSize);
	
	start = MPI_Wtime();
	for (int i = 0; i < 32; i++) {
		int base = 1 << i;
		MPI_Scatter(arr, chunkSize, MPI_INT, chunkArr, chunkSize, MPI_INT, 0, MPI_COMM_WORLD);
		int countLocal[2] = {0};
		countDigit(chunkArr, countLocal, chunkSize, base);
		
		MPI_Barrier(MPI_COMM_WORLD);
		int countGlobal[2];
		int offsetGlobal[2];
		memset(countGlobal, 0, 2 * sizeof(int));
		memset(offsetGlobal, 0, 2 * sizeof(int));
		MPI_Reduce(countLocal, countGlobal, 2, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		if (rank == 0) {
			int offsetGlobal[2] = {0};
			calculateOffsetOld(countGlobal, offsetGlobal, 2);
			radixSort(arr, countGlobal, n, offsetGlobal, 1 << i);
		}
		
		MPI_Barrier(MPI_COMM_WORLD);
	}
	stop = MPI_Wtime();
	
	free(chunkArr);
	if (rank == 0) {
		output(arr, n);
		free(arr);
		printf("Elapsed time for %d processor(s) = %lf ms\n", num_proc, (stop-start)/0.001);
	}

	MPI_Finalize();
}
