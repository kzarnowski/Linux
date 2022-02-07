#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
    int fd = open("./sukcesy", O_RDONLY);
    int res = 1;

    int index = 0;

    pid_t buffer[10];
    while (res != 0)
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

        index += 10;
        if (index > 600)
        {
            break;
        }
    }
}