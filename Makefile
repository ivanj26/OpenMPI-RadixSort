CC = mpicc

default: v1

v1 : src/main_v1.c src/util.c
	$(CC) -o bin/main src/main_v1.c src/util.c

v2 : src/main_v2.c src/util.c
	$(CC) -o bin/main src/main_v2.c src/util.c