#include <stdio.h>
#include <stdlib.h>

int main() {
	srand48(42);
	for (int i = 0; i < 5; i++) {
		printf("%lf\n", drand48());
	}
	return 0;
}
