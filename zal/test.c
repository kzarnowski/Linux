#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char *args[] = {"./p", "10", NULL};
    execvp(args[0], args);
}