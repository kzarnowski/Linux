#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define SIZE 1000

int main(int argc, char **argv)
{
    int fd = open("testfile", O_RDWR | O_CREAT | O_TRUNC | O_APPEND, S_IRWXU);

    unsigned short buffer[SIZE];
    for (int i; i < SIZE; i++)
    {
        if (i % 5 == 0)
        {
            buffer[i] = i;
        }
        else
        {
            buffer[i] = i / 10 + 5;
        }
    }

    int res = write(fd, buffer, SIZE * sizeof(unsigned short));
    if (res == -1)
    {
        perror("Writing error");
        exit(1);
    }
    close(fd);
}