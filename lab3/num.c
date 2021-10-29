#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
	if (argc > 1) {

		char* end;
		int num = strtol((const char*)(argv[argc-1]), &end, 0);
		if (*end == '\0') {
			for (int i = 1; i < argc - 1; i++) {
				if (argv[i][0] == '-') {
					switch(argv[i][1]) {
						case 'o':
							printf("%o ", num);
							break;
						case 'h':
							printf("%x ", num);
							break;
						case 'd':
							printf("%d ", num);
							break;
					}
				}
			printf("\n");	
			}
		}

	} else {
		fprintf(stderr, "Too few arguments\n");
	}
}

// dolozyc obsluge bledow dla parametrow itd
// poczytac getopt, przepisac ten program na getopt
// poczytac o opcjach typu -o <wartosc>, a nie tylko przelacznik on/off
// np po -o ma się pojawić liczba zmiennoprzecinkowa (getopt to sprawdza)
