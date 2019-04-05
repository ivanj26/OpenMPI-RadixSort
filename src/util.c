#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "util.h"

void rng(int* arr, int n, int seed) {
    srand(seed);
    for(long i = 0; i < n; i++) {
        arr[i] = (int)rand();
    }
}

void printArray(int arr[], int n) {
	for (int i = 0; i < n; i++) {
		printf("%d\n", arr[i]);
	}
}

void output(int arr[], int n) {
	chdir("../test");
	FILE *fp = fopen("output.txt", "w");

	if (fp) {
		for (int i = 0; i < n; i++) {
			fprintf(fp, "%d\n", arr[i]);
		}

		fclose(fp);
	}
}