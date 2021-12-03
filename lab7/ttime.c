#include <time.h>
#include <stdio.h>

int main() {
	time_t t1 = time(0);
	time_t t2;
	char* res = ctime(&t1);
	printf("Time: %s", res);
	struct timespec d  = {1, 750000000L};
	int r = nanosleep(&d, NULL);
	if (r < 0) {
		perror("Error while waiting\n");
	}

	t2 = time(0);
	res = ctime(&t1);
	char* res2 = ctime(&t2);

	printf("Time1: %s", res);
	printf("Time2: %s", res2);
}
