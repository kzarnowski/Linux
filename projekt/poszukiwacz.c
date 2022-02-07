#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct __attribute__((packed)) record
{
    unsigned short x;
    pid_t pid;
} RECORD;

int main(int argc, char **argv)
{
    // sprawdzenie czy jest podpiete do potoku
    struct stat stdin_stats;
    fstat(STDIN_FILENO, &stdin_stats);
    if (!S_ISFIFO(stdin_stats.st_mode))
    {
        fprintf(stderr, "STDIN is not connected to any pipe.\n");
        exit(11);
    }

    // sprawdzenie i odczytanie argumentu
    if (argc != 2)
    {
        fprintf(stderr, "Wrong number of arguments.");
        exit(12);
    }

    char *end = NULL;
    long int blok = strtol((const char *)(argv[1]), &end, 10);

    // uwzglednienie jednostki argumentu <blok>
    int multiplier = 1;
    if (end != NULL)
    {
        if (strcmp(end, "Ki") == 0)
        {
            multiplier = 1024;
        }
        else if (strcmp(end, "Mi") == 0)
        {
            multiplier = 1024 * 1024;
        }
        else if (*end != '\0')
        {
            fprintf(stderr, "Argument has a wrong format.\n");
        }
    }
    blok *= multiplier;

    // wczytywanie danych z stdin i wysyłanie na stdout
    RECORD buffer;
    buffer.pid = getpid();
    buffer.x = 0;
    int res = 0;
    unsigned short temp_x;
    int unique_values = 0;

    for (int i = 0; i < blok; i++)
    {
        res = read(STDIN_FILENO, &temp_x, sizeof(unsigned short));
        if (res == -1)
        {
            perror("Error while reading a number.");
            exit(13);
        }

        // TODO: sprawdzic sytuacje, jesli pipe nie zdazyl zostac wypelniony przez kolekcjonera

        if (temp_x < 1000 && temp_x % 3 == 0)
        {
            // tymczasowy warunek, do zastapienia przez sprawdzanie duplikatow w tablicy
            buffer.x = temp_x;
            unique_values += 1;
            write(STDOUT_FILENO, &buffer, sizeof(RECORD));
        }
    }

    // tymczasowe statusy, do zastąpienia przez
    if (unique_values > 0.8 * blok)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}