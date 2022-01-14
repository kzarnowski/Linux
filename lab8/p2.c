#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char** argv) {
	int fd = -1;
	struct timespec t = {0, 400000000L};
	char buffer[6];

	while(1) {
		fd = open(argv[1], O_RDONLY | O_NONBLOCK);
		if (fd == -1) {
			printf("Open failed\n");
			nanosleep(&t, NULL);
		} else {
			printf("Open successful\n");
			break;
		}
	}

	int len = 0;

	// read zwroci 0 jak potok bedzie zamkniety
	// read zwroci EAGAIN or EWOULDBLOCK (man 2 read) kiedy potok jest otwarty ale nic nie wpisuje

	while (len != -1) {
		len = read(fd, (void*)buffer, sizeof(buffer));
		if (len != -1) {
			printf("%s\n", buffer);
			nanosleep(&t, NULL);
		}
	}
	
	return 0;
	
}
