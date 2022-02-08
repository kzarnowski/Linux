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

int ins = 0;  // DEBUG
int dups = 0; // DEBUG

// ------------------------------------------------------------------------- //

int insert(unsigned short **data, int len, unsigned short k);
unsigned short hash(unsigned short k, int i, int len);
void free_data(unsigned short **data, int len);

// ------------------------------------------------------------------------- //

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

    // fprintf(stderr, "BLOK STRTOL: %ld\n", blok); // DEBUG
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

    // fprintf(stderr, "LEN: %d\n", len); // DEBUG

    // inicjalizacja tablicy
    for (int i = 0; i < len; i++)
    {
        data[i] = NULL;
    }

    /*
    Wykorzystuje tablice haszujaca z podwojnym haszowaniem, rozwiazywanie 
    konfliktow poprzez adresowanie otwarte. Implementacja na podstawie 
    Thomas H. Cormen - "Wprowadzenie do algorytmow, Rozdzial 11"
    */

    unsigned long int duplicates = 0;
    int res = 0;
    unsigned short temp_x;
    RECORD buffer;
    buffer.pid = getpid();

    // wczytywanie danych z stdin i wysyłanie na stdout, jesli liczba pojawila
    // sie po raz pierwszy

    for (int i = 0; i < blok; i++)
    {
        // if (i % 100 == 0 || i > 32700)  // DEBUG
        //     fprintf(stderr, "%d\n", i); // DEBUG
        res = read(STDIN_FILENO, &temp_x, sizeof(unsigned short));
        if (res == -1)
        {
            perror("Error while reading a number.");
            exit(13);
        }

        // TODO: sprawdzic sytuacje, jesli pipe nie zdazyl zostac wypelniony przez
        // kolekcjonera (tryb blokujacy, wiec chyba nic nie trzeba robic?)

        res = insert(data, len, temp_x);
        if (res == -1)
        {
            dups++; // DEBUG
            duplicates += 1;
        }
        else if (res == 0)
        {
            ins++; // DEBUG
            buffer.x = temp_x;
            write(STDOUT_FILENO, &buffer, sizeof(RECORD));
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

    fprintf(stderr, "INS: %d, DUPS: %d.\n", ins, dups); // DEBUG
    return result;
}

unsigned short hash(unsigned short k, int i, int len)
{
    return (k + i) % len;
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
    fprintf(stderr, "INS: %d, DUPS: %d.\n", ins, dups);
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