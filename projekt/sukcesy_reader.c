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
    int successes = 0;
    int zeros = 0;

    off_t end = lseek(fd, 0, SEEK_END);
    off_t start = lseek(fd, 0, SEEK_SET);

    printf("ROZMIAR PLIKU: %ld\n", end);

    int index = 0;
    pid_t buffer[10];
    while (start < end)
    {
        res = read(fd, buffer, sizeof(buffer));
        if (res == -1)
        {
            perror("Read error");
            exit(1);
        }
        for (int i = 0; i < 10; i++)
        {
            if (buffer[i] > 0)
            {
                successes++;
            }
            else
            {
                zeros++;
            }
        }

        index += 10;

        start += sizeof(buffer);
    }
    printf("INDEKS NAJWIEKSZEJ LICZBY +/- 10: %d\n", index);
    printf("LICZBA WPISANYCH PIDOW: %d\n", successes);
    printf("LICZBA ZER: %d\n", zeros);
    printf("LICZBA ZER + LICZBA PIDOW: %d\n", successes + zeros);
}