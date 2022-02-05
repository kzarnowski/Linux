#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define N 100           // TEMPORARY
#define MAX_SHORT 32767 // (2^16)/2 - 1

typedef struct __attribute__((packed)) record
{
    short k;
    pid_t pid;
} RECORD;

/*
    RETURN VALUES:
    11 - ...
    12 - ...
*/

int insert(unsigned short **data, int len, unsigned short k);
void free_data(unsigned short **data, int len);

int main(int argc, char **argv)
{
    struct stat stdin_stats;
    fstat(STDIN_FILENO, &stdin_stats);
    if (!S_ISFIFO(stdin_stats.st_mode)) // CHECK
    {
        fprintf(stderr, "STDIN is not connected to any pipe.\n");
        exit(15);
    }

    if (argc != 2)
    {
        fprintf(stderr, "Wrong number of arguments.");
        exit(11);
    }

    char *end = NULL;
    long int num = strtol((const char *)(argv[1]), &end, 10);

    if (num <= 0)
    {
        fprintf(stderr, "Argument must be a positive number.\n");
        exit(12);
    }

    int multiplier = 1;
    if (end != NULL)
    {
        // Zostala podana jednostka.
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

    num *= multiplier;

    /*
    Uznajemy, ze wczytywane liczby musza byc nieujemne, aby mozna je bylo wykorzystac w drugim programie
    do indeksacji w pliku sukcesow. Tablica bedzie przechowywac tylko unikalne wartosci, wiec jej rozmiar
    nie musi byc wiekszy niz najwieksza dodatnia liczba 2-bajtowa - potencjalna oszczednosc pamieci.
    */

    int len = num < MAX_SHORT ? num : MAX_SHORT;
    unsigned short **data = (unsigned short **)malloc(len * sizeof(unsigned short *));
    if (data == NULL)
    {
        perror("Data array memory allocation error.");
        exit(13);
    }

    for (int i = 0; i < len; i++)
    {
        data[i] = NULL;
    }

    /*
    Wykorzystuje tablice haszujaca z podwojnym haszowaniem, rozwiazywanie konfliktow poprzez adresowanie otwarte.
    Implementacja na podstawie Thomas H. Cormen - "Wprowadzenie do algorytmow, Rozdzial 11"
    */

    unsigned long int duplicates = 0;

    unsigned short input[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

    int res = 0;
    RECORD buffer;
    for (int i = 0; i < 10; i++)
    {
        res = insert(data, len, input[i]);
        if (res == -1)
        {
            duplicates += 1;
        }
        else if (res == 0)
        {
            buffer.k = input[i]; // TEMPORARY
            buffer.pid = getpid();
            write(STDOUT_FILENO, &buffer, sizeof(buffer));
        }
    }

    printf("TABLE: %ld\n", duplicates);
    unsigned short tmp;
    for (int i = 0; i < len; i++)
    {
        tmp = data[i] != NULL ? *data[i] : 0;
        printf("%d ", tmp);
    }

    free_data(data, len);
}

/*
    data - tablica
    k - wstawiana liczba
    len - rozmiar tablicy
    i - zmienna pomocniczna, iteracja przy haszowaniu
*/

unsigned short hash(unsigned short k, int i, int len)
{
    unsigned short h1 = k % len;
    unsigned short h2 = 1 + (k % (len - 1));
    return (h1 + i * h2) % len;
}

int insert(unsigned short **data, int len, unsigned short k)
{
    int i = 0;
    int j = 0;
    while (i != len)
    {
        j = hash(k, i, len);
        if (data[j] == NULL)
        {
            // nowy, unikalny element
            data[j] = (unsigned short *)malloc(sizeof(unsigned short));
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