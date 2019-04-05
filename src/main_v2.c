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
		int countGlobal[2 * num_proc];
		int offsetGlobal[2 * num_proc];
		memset(countGlobal, 0, 2 * num_proc * sizeof(int));
		memset(offsetGlobal, 0, 2 * num_proc * sizeof(int));

		MPI_Gather(countLocal, 2,  MPI_INT, countGlobal, 2, MPI_INT, 0, MPI_COMM_WORLD);
		if (rank == 0) {
			calculateOffset(countGlobal, num_proc, offsetGlobal, 2 * num_proc);
		}
		
		MPI_Barrier(MPI_COMM_WORLD);
		MPI_Bcast(offsetGlobal, 2 * num_proc, MPI_INT, 0, MPI_COMM_WORLD);	

		if (rank == 0) {
			int* output = malloc(n * sizeof(int));
			memset(output, 0, n * sizeof(int));
			for (int i = 0; i < chunkSize; i++) {
				int bit = (chunkArr[i] & base) ? 1 : 0;
				MPI_Send(&chunkArr[i], 1, MPI_INT, 0, offsetGlobal[rank + (bit * num_proc)]++, MPI_COMM_WORLD);
			}
			for (int i = 0; i < n; i++) {
				MPI_Recv(&output[i], 1, MPI_INT, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
			memcpy(arr, output, n * sizeof(int));
			free(output);
		} else {
			for (int i = 0; i < chunkSize; i++) {
				int bit = (chunkArr[i] & base) ? 1 : 0;
				MPI_Send(&chunkArr[i], 1, MPI_INT, 0, offsetGlobal[rank + (bit * num_proc)]++, MPI_COMM_WORLD);
			}
		}

		MPI_Barrier(MPI_COMM_WORLD);
	}
	stop = MPI_Wtime();
	if (rank == 0) {
		output(arr, n);
		printf("Elapsed time for %d processor(s) = %lf ms\n", num_proc, (stop-start)/0.001);
	}

	MPI_Finalize();
}
