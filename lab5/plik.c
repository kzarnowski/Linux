// wczytac 64 bajty z urandom
// wypisać wartoci HEX, po 8 znaków na linię, %hhx => h oznacza połowe z zadanej wielkosci
//
// potem wypisac wartosci HEX interpretowane jako short, czyli %hx
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#define N 64
#define ROW_SIZE 8

int main() {
	char T[N];
	int fd = open("/dev/urandom", O_RDONLY);
	ssize_t len = read(fd, (void*)T, N);
	if (len < N) {
		perror("Read error\n");
		exit(EXIT_FAILURE);
	}	

	for (int i = 0; i < N; i++) {
		printf("%02hhx ", T[i]);
		if (i % ROW_SIZE == ROW_SIZE - 1) {
			printf("\n");
		}
	}
	
	printf("\n\n");

	unsigned short* s = T;
	int size = sizeof(unsigned short);

	for (int i = 0; i < N/size; i++) {
		printf("%04hx ", *s);
		if (i % (ROW_SIZE / size)  == (ROW_SIZE / size) - 1) {
			printf("\n");
		}
		s++;
	}

	printf("\n\n");
	
	unsigned int* x = T;
	size = sizeof(unsigned int);

	for (int i = 0; i < N/size; i++) {
		printf("%08x ", *x);
		if (i % (ROW_SIZE / size)  == (ROW_SIZE / size) - 1) {
			printf("\n");
		}
		x++;
	}

}
