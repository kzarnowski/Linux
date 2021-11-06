#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//COLORS
#define RED "\e[31m"
#define GRN "\e[32m"
#define RESET "\e[0m"

int main() {
    
    char tab[200];
    char* names[] = { "ABC", "BMD", "CDX", "ASD", "WDI", "ZZZ" }; // length(names) == N
    //char* format = "%s: %s%.2lf\e[0m, ";
	char* format = "%s: %.2lf, ";

#define N  sizeof(names)/sizeof(char*)

    srand48(42);
    double num[N];
    for (int i = 0; i < N; i++) {
    	num[i] = 1000 * drand48();
    }
    
    int idx = 0;
    for (int i = 0; i < N; i++) {
        idx += snprintf(tab + idx, 200 - idx, format, names[i], num[i] /*> 500 ? GRN : RED, num[i]*/);
    }
    //tab[idx-2] = '\n';
    //tab[idx-1] = '\0';
   
	int start_idx = 0;
	int tail_len;
	while(1) {
		tail_len = printf("%.40s\r", &tab[start_idx]);
		fflush(stdout);
		start_idx = (start_idx + 1) % 40;
		usleep(1000000);
	}	
}
