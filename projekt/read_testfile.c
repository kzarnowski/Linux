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

    int fd = open("testfile", O_RDONLY);
    unsigned short buffer[10];
    off_t end = lseek(fd, 0, SEEK_END);
    off_t start = lseek(fd, 0, SEEK_SET);

    while (start < end)
    {
        int res = read(fd, buffer, sizeof(buffer));
        if (res == -1)
        {
            perror("Read error");
            exit(1);
        }
        for (int i = 0; i < 10; i++)
        {
            printf("%d ", buffer[i]);
        }
        printf("\n");

        start += sizeof(buffer);
    }
    close(fd);
}
