#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define SIZE 100

int main()
{
    int fd = open("tekst", O_RDWR | O_CREAT | O_TRUNC | O_APPEND, S_IRWXU);

    char buffer[SIZE];
    for (int i = 0; i < SIZE; i++)
    {
        buffer[i] = 'A';
    }

    int res = write(fd, buffer, 5);
    printf("RES: %d", res);
}