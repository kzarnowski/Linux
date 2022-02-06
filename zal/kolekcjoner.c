#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define NANOSEC 1000000000L
#define FL2NANOSEC(f)                          \
    {                                          \
        (long)(f), ((f) - (long)(f)) * NANOSEC \
    }

char *source_path;             // -d <zrodlo>    dane do pobrania
unsigned long int total_len;   // -s <wolumen>   ilosc danych
unsigned long int segment_len; // -w <blok>      ilosc danych na potomka
char *success_path;            // -f <sukcesy>   sciezka do pliku osiagniec
char *report_path;             // -l <raporty>   sciezka do pliku raportow
unsigned int childs_num;       // -p <prac>      maksymalna liczba potomkow

int parent_to_child[2]; // rodzic wysyla, potomek czyta
int child_to_parent[2]; // rodzic czyta, potomek wysyla

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

    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1)
    {
        perror("Creating pipe error.\n");
        exit(2);
    }

    int pid;
    for (int i = 0; i < 10 /*childs_num*/; i++)
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
        printf("CHILD FUNC\n");
        child();
    }
    else
    {
        printf("PARENT FUNC\n");
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
    char *args[] = {"./p", "10", NULL};
    execvp(args[0], args);

    printf("EXEC error\n");

    close(parent_to_child[1]);
    close(child_to_parent[0]);
    // write: child_to_parent, read: parent_to_child
    dup2(parent_to_child[0], STDIN_FILENO);
    dup2(child_to_parent[1], STDOUT_FILENO);

    close(parent_to_child[0]);
    close(child_to_parent[1]);

    const struct timespec t = FL2NANOSEC(0.2);
    int buffer;
    int message[2];
    message[0] = -1;
    message[1] = getpid();
    while (1)
    {
        int res = read(STDIN_FILENO, &buffer, sizeof(int));
        if (res != -1)
        {
            message[0] = buffer;
            write(STDOUT_FILENO, message, sizeof(message));
        }
        nanosleep(&t, NULL);
    }
}

void parent()
{

    close(parent_to_child[0]);
    close(child_to_parent[1]);
    // write: parent_to_child, read: child_to_parent

    int buffer[2];
    int i = 0;

    int msg[100];
    for (int i = 0; i < 20; i++)
    {
        msg[i] = i;
    }

    const struct timespec t = FL2NANOSEC(0.5);

    // na podstawie: https://www.linuxtoday.com/blog/blocking-and-non-blocking-i-0/
    fcntl(parent_to_child[1], F_SETFL, fcntl(parent_to_child[1], F_GETFL) | O_NONBLOCK); // add error checking
    fcntl(child_to_parent[0], F_SETFL, fcntl(child_to_parent[0], F_GETFL) | O_NONBLOCK); // add error checking

    while (1)
    {
        nanosleep(&t, NULL);
        write(parent_to_child[1], msg + i, sizeof(int));
        read(child_to_parent[0], buffer, sizeof(buffer));
        printf("I: %d, PID: %d\n", buffer[0], buffer[1]);
        i++;
    }

    /*

    struct record = {k, pid}
    char* buffer = ...
    while() {
        read(source_path, buffer)
        write(parent_to_child[1], buffer)
        read(child_to_parent[0], record)
        
        res1 = write(success_path, record)
            75% wypelnienia : czyli wpisano 0.75 * MAX_SHORT rekordow
        res2 = // CHECK IF ANY CHILD HAS DIED
            umarl AND <75% : utworz nowego potomka

        if (res1 == -1 AND res2 == -1) {
            // both failed
            sleep(0.48)
        }

    }

    */
}