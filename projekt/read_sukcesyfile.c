#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SIZE 1000

int main()
{
    int fd = open("./sukcesy", O_RDONLY);
    int res = 1;

    pid_t buffer;
    for (int i = 0; i < SIZE; i++)
    {
        res = read(fd, &buffer, sizeof(pid_t));
        if (res == -1)
        {
            perror("Read error");
            exit(1);
        }
        printf("%d ", buffer);

        if (i % 10 == 0)
        {
            printf("\n");
        }
    }
}