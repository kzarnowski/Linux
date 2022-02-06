#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

char *source_path;             // -d <zrodlo>    dane do pobrania
unsigned long int total_len;   // -s <wolumen>   ilosc danych
unsigned long int segment_len; // -w <blok>      ilosc danych na potomka
char *success_path;            // -f <sukcesy>   sciezka do pliku osiagniec
char *report_path;             // -l <raporty>   sciezka do pliku raportow
unsigned int childs_num;       // -p <prac>      maksymalna liczba potomkow

int read_args(int argc, char **argv);
void child();
void parent();

int main(int argc, char **argv)
{
    if (read_args(argc, argv) != 0)
    {
        fprintf(stderr, "Reading arguments error.\n");
        exit(1);
    }

    // printf(
    //     "-d SOURCE_PATH: %s\n"
    //     "-s TOTAL_LEN: %ld\n"
    //     "-w SEGMENT_LEN: %ld\n"
    //     "-f SUCCESS_PATH: %s\n"
    //     "-l REPORT_PATH: %s\n"
    //     "-p CHILDS_NUM: %d\n",
    //     source_path, total_len, segment_len, success_path, report_path, childs_num);

    int pid;
    for (int i = 0; i < childs_num; i++)
    {
        pid = fork();

        if (pid == -1)
        {
            perror("Creating a child error.");
            exit(2);
        }
        else if (pid == 0)
        {
            break;
        }
    }

    if (pid == 0)
    {
        child();
    }
    else
    {
        parent();
    }
}

int read_args(int argc, char **argv)
{

    int option;
    char *end;
    char *invalid_arg = NULL;

    while ((option = getopt(argc, argv, "d:s:w:f:l:p:")) != -1)
    {
        switch (option)
        {
        case 'd':
            source_path = optarg;
            break;
        case 's':
            total_len = strtoul(optarg, &end, 10);
            if (*end != '\0')
            {
                invalid_arg = optarg;
            }
            break;
        case 'w':
            segment_len = strtoul(optarg, &end, 10);
            if (*end != '\0')
            {
                invalid_arg = optarg;
            }
            break;
        case 'f':
            success_path = optarg;
            break;
        case 'l':
            report_path = optarg;
            break;
        case 'p':
            childs_num = (unsigned int)strtoul(optarg, &end, 10);
            if (*end != '\0')
            {
                invalid_arg = optarg;
            }
            break;
        case '?':
            return 1;
        }

        if (invalid_arg != NULL)
        {
            fprintf(stderr, "Invalid argument: %s. ", invalid_arg);
            return 1;
        }
    }

    // Ponizsze sprawdzanie zapozyczone z manuala.
    if (optind < argc)
    {
        fprintf(stderr, "Too many arguments. ");
        return 1;
    }

    return 0;
}

void child()
{
}

void parent()
{
}