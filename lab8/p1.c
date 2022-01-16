#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SEC 1000000000L

int main(int argc, char** argv) {
	struct timespec t = {1, 0.5 * SEC};
	nanosleep(&t, NULL);
	int fd = open(argv[1], O_WRONLY);

	if (fd == -1) {
		perror("Opening error");
		exit(1);
	}

	char message[] = "Hello";
	int len = 0;
	struct timespec t2 = {0, 0.75 * SEC};

	for (int i = 0; i <5; i++) {
		nanosleep(&t2, NULL);
		len = write(fd, message, sizeof(message));
		if (len == -1) {
			perror("Writing error");
			exit(1);
		}
	}
}
