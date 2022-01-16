#include <stdlib.h>
#include <stdio.h>

void f(void) {
    printf("f was called\n");
}

void g(int status, void* arg) {
    printf("g was called: status %d arg: %ld\n", status, (long)arg);
}

int main(int argc, char** argv) {
    atexit(f);
    on_exit(g, (void*)10);

    printf("Hello\n");
    exit(2);
}