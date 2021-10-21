#include <stdio.h>
#include <stdlib.h>

#define N 5

// COLORS
#define RED "\e[31m"
#define GRN "\e[32m"
#define YEL "\e[33m"
#define BLU "\e[34m"
#define MAG "\e[35m"
#define CYN "\e[36m"
#define WHT "\e[37m"
#define RESET "\e[0m"

int main() {
    srand48(42);
    double num[N];
    for (int i = 0; i < N; i++) {
        num[i] = 1000 * drand48();
    }
    
    char tab[60];
    char* names[] = { "ABC", "BMD", "CDX", "ASD", "WDI" }; // length(names) == N
    char* format = "%s: %.2lf, ";
    
    int idx = 0;
    for (int i = 0; i < N; i++) {
        idx += snprintf(tab + idx, 60, format, names[i], num[i]);
    }
    tab[idx-2] = '\n';
    tab[idx-1] = '\0';
    
    printf("%s", tab);
    printf(RED "xxxx\n" RESET);
    printf(YEL "yyy\n" RESET);
}
