#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_SHORT 65535 // 2^16 - 1

typedef struct __attribute__((packed)) record
{
    unsigned short x;
    pid_t pid;
} RECORD;

// ------------------------------------------------------------------------- //

int insert(unsigned short **data, int len, unsigned short k);
unsigned short hash(unsigned short k, int i, int len);
void free_data(unsigned short **data, int len);

// ------------------------------------------------------------------------- //

int main(int argc, char **argv)
{
    int debug_fd = open("./debug_file", O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IRWXU); // DEBUG

    // sprawdzenie czy jest podpiete do potoku
    struct stat stdin_stats;
    fstat(STDIN_FILENO, &stdin_stats);
    if (!S_ISFIFO(stdin_stats.st_mode))
    {
        fprintf(stderr, "%d STDIN is not connected to any pipe.\n", getpid());
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

    /*
    Tablica bedzie przechowywac tylko unikalne wartosci zmiennej unsigned short, 
    wiec jej rozmiar nie musi byc wiekszy niz najwieksza dodatnia liczba 2-bajtowa
    + 1 miejsce na liczbe 0
    */

    int len = blok < MAX_SHORT ? blok : MAX_SHORT + 1;
    unsigned short **data = (unsigned short **)malloc(
        len * sizeof(unsigned short *));

    if (data == NULL)
    {
        perror("Data array memory allocation error.");
        exit(13);
    }

    // inicjalizacja tablicy
    for (int i = 0; i < len; i++)
    {
        data[i] = NULL;
    }

    /*
    Wykorzystuje tablice haszujaca, rozwiazywanie konfliktow poprzez 
    adresowanie otwarte. Implementacja na podstawie 
    Thomas H. Cormen - "Wprowadzenie do algorytmow, Rozdzial 11"
    */

    unsigned long int duplicates = 0;
    int res = 0;
    unsigned short temp_x;
    RECORD buffer;
    buffer.pid = getpid();

    // wczytywanie danych z stdin i wysyłanie na stdout, jesli liczba pojawila
    // sie po raz pierwszy

    int write_res;
    for (int i = 0; i < blok; i++)
    {
        res = read(STDIN_FILENO, &temp_x, sizeof(unsigned short));
        dprintf(debug_fd, "POSZUKIWACZ PID: %d PRZECZYTAL %d\n", getpid(), res); // DEBUG
        if (res == -1)
        {
            perror("Error while reading a number.");
            exit(13);
        }
        else if (res == 0)
        {
            // pipe zostal zamkniety
            return 13;
        }

        res = insert(data, len, temp_x);
        if (res == -1)
        {
            duplicates += 1;
        }
        else if (res == 0)
        {
            buffer.x = temp_x;
            write_res = write(STDOUT_FILENO, &buffer, sizeof(RECORD));
            if (write_res == -1)
            {
                perror("Write error");
                exit(11);
            }
        }
    }

    free_data(data, len);

    // obliczenie statusu zakonczenia
    int ratio = (int)(((double)duplicates / len) * 1000); // ex. ratio 100 == 10%
    int result = ratio / 100;

    if (ratio % 100 > 0)
    {
        result++;
    }

    return result;
}

unsigned short hash(unsigned short k, int i, int len)
{
    return (k + i) % len;
}

int insert(unsigned short **data, int len, unsigned short k)
{
    // zwraca 0 jesli element jest unikalny, -1 jesli duplikat
    int i = 0;
    int j = 0;
    while (i != len)
    {
        j = hash(k, i, len);
        if (data[j] == NULL)
        {
            // nowy, unikalny element
            data[j] = (unsigned short *)malloc(sizeof(unsigned short));
            if (data[j] == NULL)
            {
                fprintf(stderr, "Hash table malloc error.\n");
                exit(14);
            }
            *data[j] = k;
            return 0;
        }
        else if (*data[j] == k)
        {
            // duplikat
            return -1;
        }
        else
        {
            // konflikt funkcji haszujacej, ale liczby sie roznia, szukamy nowego miejsca
            i += 1;
        }
    }
    /* 
    Z uwagi na specyfike wstawianych danych i rozmiar tablicy, teoretycznie
    nie powinno dojsc do jej przepelnienia (nigdy nie bedziemy miec do wstawienia
    wiecej elementow niz rozmiar tablicy). Mimo wszystko nalezy sie zabezpieczyc.
    */
    fprintf(stderr, "Hash table overflow.\n");
    free_data(data, len);
    exit(14);
}

void free_data(unsigned short **data, int len)
{
    for (int i = 0; i < len; i++)
    {
        free(data[i]);
    }
    free(data);
}