#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

int main (int argc, char** argv) {
	if (argc > 1) {
		char* end;
		int num = strtol((const char*)(argv[argc-1]), &end, 0);	
		int option;
		while((option = getopt(argc, argv, "ohd")) != -1) {
			switch (option) {
				case 'o':
					printf("%o ", num);
					break;
				case 'h':
					printf("%x ", num);
					break;
				case 'd':
					printf("%d ", num);
					break;
				default:
					fprintf(stderr, "Invalid argument.");
			}
			printf("\n");
		}
	} else {
		fprintf(stderr, "Arguments were not provided.\n");
	}
}
