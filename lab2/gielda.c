#include <stdio.h>
#include <stdlib.h>

int main() {
	srand48(42);
	double num[5];
	for (int i = 0; i < 5; i++) {
		num[i] = 1000 * drand48();
	}

	char* format = "ABC: %.2lf, BMD: %.2lf, CDX: %.2lf, ASD: %.2lf, WDI: %.2lf\n";
	char tab[60];
	snprintf(tab, 60, format, num[0], num[1], num[2], num[3], num[4]);
	printf("%s", tab);

}
