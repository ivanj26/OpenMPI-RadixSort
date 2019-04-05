#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define DIGIT 10
#define FILENAME "./test/output.txt"

int pow_ten[10] = {
    1,		10,		100,
	1000,		10000,		100000,
	1000000,	10000000,	100000000,
	1000000000,
};

void generate_array(int *arr, int n);
void output_to_file(int *arr, int n);
void print_array(int *arr, int n);
void radix_sort(int *arr, int n, int rank, int num_proc, int *global_arr);
void count_sort(int *arr, int n, int exp);
int get_max_value(int *arr, int n);
int get_max_digit(int m);
int get_max_parallel(int *arr, int n);

int main(int argc,char *argv[]) {
    double start, stop;
    int num_proc, rank, n;
    int *arr;

    //Check if argument is available or not
    assert(argc > 1);

    n = atoi(argv[1]);

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    arr = NULL;
    if (rank == 0){
        arr = (int*) malloc(sizeof(int) * n);
        assert(arr != NULL);

        //Generate pseudo random elements for array arr */
        generate_array(arr, n);

        // printf("Before sorting: \n");
        // print_array(arr, n);
    }

    int local_size = n / num_proc;
    int *local_arr = (int*) malloc(sizeof(int) * local_size);

    assert(local_arr != NULL);

    start = MPI_Wtime();

    MPI_Scatter(arr, local_size, MPI_INT, local_arr, local_size, MPI_INT, 0, MPI_COMM_WORLD);

    radix_sort(local_arr, local_size, rank, num_proc, arr);

    stop = MPI_Wtime();

    if (rank == 0){
        printf("After sorting: \n");
        print_array(arr, n);

        printf("Elapsed time for %d processor(s) = %lf ms\n", num_proc, (stop-start)/0.001);

        output_to_file(arr, n);
        free(arr);
    }
    free(local_arr);

    MPI_Finalize();
}

void radix_sort(int *arr, int n, int rank, int num_proc, int* global_arr){
    int m = get_max_parallel(arr, n);
    int i, j, k, max_digit;

    if (0 == rank){
        max_digit = get_max_digit(m);
    }

    MPI_Bcast(&max_digit, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Initialize local bucket
    int local_bucket[max_digit][DIGIT];
    for (i = 0; i < max_digit; i++) {
        for (j = 0; j < DIGIT; j++) {
            local_bucket[i][j] = 0;
        }
    }

    //Counting occurence
    for (i = 0; i < max_digit; i++) {
        for (j = 0; j < n; j++) {
            int pow = pow_ten[i];
            int idx = (arr[j]/pow) % DIGIT;

            local_bucket[i][idx]++;
        }

        for (k = 1; k < DIGIT; k++)
            local_bucket[i][k] += local_bucket[i][k-1];
    }

    int global_count[max_digit][DIGIT];
    MPI_Reduce(local_bucket, global_count, max_digit*DIGIT, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (0 == rank){
        int total = num_proc * n;
        int output[total];

        for (i = 0; i < max_digit; i++) {
            int pow = pow_ten[i];

            for (j = total-1; j >= 0; j--) {
                int idx = (global_arr[j] / pow) % DIGIT;

                output[global_count[i][idx] - 1] = global_arr[j];
                global_count[i][idx]--;
            }

            for (k = 0; k < total; k++) {
                global_arr[k] = output[k];
            }
        }
    }
}

int get_max_parallel(int *arr, int n){
    int local_max = get_max_value(arr, n);

    int global_max;
    MPI_Reduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    return global_max;
}

int get_max_digit(int m){
    int max_digit = 1;
    while ((m /= 10) > 0){
        max_digit++;
    }

    return max_digit;
}

void output_to_file(int *arr, int n){
    FILE *f;
	f = fopen(FILENAME, "w+");

	int i;

	for (i = 0; i < n-1; i++){
		fprintf(f, "%d\n", arr[i]);
	}
	fprintf(f, "%d", arr[n-1]);

	fclose(f);
}

void print_array(int *arr, int n){
    for (int i = 0; i < n; i++) {
        printf("%d\n", arr[i]);
    }
    printf("\n");
}

void generate_array(int *arr, int n){
    int seed = 13516059;
    srand(seed);
    for (int i = 0; i < n; i++) {
        arr[i] = rand();
    }
}

int get_max_value(int *arr, int n){
    int max = arr[0];
    for (int i = 1; i < n; i++) {
        if (max < arr[i])
            max = arr[i];
    }

    return max;
}
