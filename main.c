#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void generate_array(int *arr, int n);
long compute_sum(int *arr, int n, int rank);
void print_array(int *arr, int n);
void radix_sort(int *arr, int n, long max_value);
int get_max_value(int *arr, int n);
int get_max_digit(int max_value);

int main(int argc,char *argv[]) {
    //Check if argument is available or not
    assert(argc > 1);

    int n = atoi(argv[1]);

    int num_proc, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int *arr = NULL;
    if (rank == 0){
        arr = (int*) malloc(sizeof(int) * n);
        assert(arr != NULL);

        //Generate pseudo random elements for array arr */
        generate_array(arr, n);

        printf("All elements: \n");
        print_array(arr, n);
    }

    int sub_arr_size = n / num_proc;
    int *sub_arr = (int*) malloc(sizeof(int) * sub_arr_size);

    assert(sub_arr != NULL);
    MPI_Scatter(arr, sub_arr_size, MPI_INT, sub_arr, sub_arr_size, MPI_INT, 0, MPI_COMM_WORLD);

    long sub_sum = compute_sum(sub_arr, sub_arr_size, rank);
	printf("> %ld\n", sub_sum);
    long *sub_sums = NULL;

    if (rank == 0){
        sub_sums = (long*) malloc(sizeof(long) * num_proc);
        assert(sub_sums != NULL);
    }

    MPI_Gather(&sub_sum, 1, MPI_LONG, sub_sums, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    if (rank == 0){
        printf("\nStarting sum\n\n");
        long sum = 0;
        for (int i = 0; i < num_proc; i++) {
            sum+=sub_sums[i];
        }
        printf("Sum of all elements: %ld\n", sum);

        free(arr);
        free(sub_sums);
    }
    free(sub_arr);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}

void print_array(int *arr, int n){
    for (int i = 0; i < n; i++) {
        printf("%d\n", arr[i]);
    }
    printf("\n");
}

long compute_sum(int *arr, int n, int rank){
    printf("#%d -> n: %ld\n", rank, n);
    printf("#%d -> %ld\n", rank, arr[n-1]);
    if (n > 1){
        return arr[n-1] + compute_sum(arr, n-1, rank);
    } else {
        return arr[n-1];
    }
}

void generate_array(int *arr, int n){
    int seed = 13516059;
    srand(seed);
    for (int i = 0; i < n; i++) {
        arr[i] = rand();
    }
}

int get_max_digit(int max_value){
    int digit = 1;

    while ((max_value /= 10) > 0){
        digit++;
    }

    return digit;
}

int get_max_value(int *arr, int n){
    int max = arr[0];
    for (int i = 1; i < n; i++) {
        if (max < arr[i])
            max = arr[i];
    }

    return max;
}
