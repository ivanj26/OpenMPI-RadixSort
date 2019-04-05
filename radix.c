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
		// arr[0] = 4;
		// arr[1] = 2;
		// arr[2] = 1;
		// arr[3] = 8;
		// arr[4] = 1;
		// arr[5] = 8;
		assert(arr != NULL);
		rng(arr, n, 13516059);

		// printf("All elements: \n");
		// print_array(arr, n);	
	}

	int chunkSize = n / num_proc;
	int* chunkArr = (int*) malloc(sizeof(int) * chunkSize);
	
	// printf("MyRank %d\n", rank);
	// printf("Before:\n");
	// print_array(chunkArr, chunkSize);
	start = MPI_Wtime();
	for (int i = 0; i < 32; i++) {
		int base = 1 << i;
		MPI_Scatter(arr, chunkSize, MPI_INT, chunkArr, chunkSize, MPI_INT, 0, MPI_COMM_WORLD);
		int countLocal[2] = {0};
		countDigit(chunkArr, countLocal, chunkSize, base);
		// print_array(countLocal, 2);
		
		MPI_Barrier(MPI_COMM_WORLD);
		int countGlobal[2 * num_proc];
		int offsetGlobal[2 * num_proc];
		memset(countGlobal, 0, 2 * num_proc * sizeof(int));
		memset(offsetGlobal, 0, 2 * num_proc * sizeof(int));
		// int countGlobal[2];
		// int offsetGlobal[2];
		// memset(countGlobal, 0, 2 * sizeof(int));
		// memset(offsetGlobal, 0, 2 * sizeof(int));
		// MPI_Reduce(countLocal, countGlobal, 2, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		MPI_Gather(countLocal, 2,  MPI_INT, countGlobal, 2, MPI_INT, 0, MPI_COMM_WORLD);
		if (rank == 0) {
			printf("countGlobal: \n");
			print_array(countGlobal, 2 * num_proc);
			calculateOffset(countGlobal, num_proc, offsetGlobal, 2 * num_proc);
			printf("offsetGlobal: \n");
			print_array(offsetGlobal, 2 * num_proc);
			// radixSort(arr, countGlobal, n, offsetGlobal, 1 << i);
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
		// for (int i = 0; i < chunkSize; i++) {
		// 	int bit = (chunkArr[i] & base) ? 1 : 0;
		// 	output[ offsetGlobal[rank + (bit * num_proc)]++ ] = chunkArr[i];
		// }
		// MPI_Barrier(MPI_COMM_WORLD);
		// printf("output rank-%d:\n", rank);
		// print_array(output, n);
		// MPI_Barrier(MPI_COMM_WORLD);
		// memcpy(arr, output, n * sizeof(int));
		// free(output);
		
		// if (rank == 0) {
		// 	printf("array:\n");
		// 	print_array(arr, n);
		// }
		// int offsetLocal[2] = {0};
		// calculateOffset(countLocal, offsetLocal, 2);
		// print_array(offsetLocal, 2);

		// radixSort(chunkArr, countLocal, chunkSize, offsetLocal, 1 << i);
	}
	stop = MPI_Wtime();
	if (rank == 0) {
		printf("After:\n");
		print_array(arr, n);
		printf("Elapsed time for %d processor(s) = %lf ms\n", num_proc, (stop-start)/0.001);
	}

	// free(chunkArr);
	MPI_Finalize();
}
