#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	printf("Num of args: %d\n", argc - 1);
	char* end;
	for (int i = 1; i < argc; i++) {
		int number = strtol((const char*)(argv[i]), &end, 0);
	       	if (*end == '\0') {
			printf(" NUM  -  %d\n", number);
		} else if (argv[i][0] == '-') {
			printf("  -  OPT %s\n", argv[i]);
		} else {
			printf("  -   -  %s\n", argv[i]);
		}	
		
		//printf("  Arg%d: %s\n", i, argv[i]);
	}
}
