#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

int debug_fd; // DEBUG

#define NANOSEC 1000000000L
#define FL2NANOSEC(f)                          \
    {                                          \
        (long)(f), ((f) - (long)(f)) * NANOSEC \
    }

typedef struct __attribute__((packed)) record
{
    unsigned short x;
    pid_t pid;
} RECORD;

#define DATA_BUFFER_SIZE 1000
#define MAX_USHORT 65535 // 2^16 - 1

// ------------------------------------------------------------------------- //

char *zrodlo;              // -d <zrodlo>    dane do pobrania
char *sukcesy;             // -f <sukcesy>   sciezka do pliku osiagniec
char *raporty;             // -l <raporty>   sciezka do pliku raportow
unsigned long int wolumen; // -s <wolumen>   ilosc danych
unsigned long int blok;    // -w <blok>      ilosc danych na potomka
char *blok_str;
unsigned long int prac; // -p <prac>      maksymalna liczba potomkow

int parent_to_child[2]; // rodzic wysyla, potomek czyta
int child_to_parent[2]; // rodzic czyta, potomek wysyla

int death_counter = 0;
int birth_counter = 0;
int kids_alive;
int num_of_successes = 0;

// ------------------------------------------------------------------------- //

void child();
void parent(int zrodlo_fd, int sukcesy_fd, int raporty_fd);
void signal_register(int signum, void *func, struct sigaction *sa);
void write_report(int raporty_fd, int pid, char *note, struct timespec *t);
void initialize_report_headers(int raporty_fd);
int handle_deaths(int raporty_fd, int pipe_open);
void open_files(int *zrodlo_fd, int *sukcesy_fd, int *raporty_fd);
int create_first_kids(int raporty_fd);
void set_nonblock_mode();
int read_args(int argc, char **argv);
void initialize_zeros(int sukcesy_fd);

void read_data(int zrodlo_fd, unsigned short *data_buffer);
int send_data(unsigned short *data_buffer);
void read_record();
void save_record(int sukcesy_fd, RECORD *record_buffer);

// ------------------------------------------------------------------------- //

int main(int argc, char **argv)
{
    debug_fd = open("./debug_file", O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IRWXU); // DEBUG
    if (debug_fd == -1)
    {
        perror("DEBUG FILE ERROR\n");
        exit(1);
    }

    if (read_args(argc, argv) != 0)
    {
        fprintf(stderr, "Reading arguments error.\n");
        exit(1);
    }

    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1)
    {
        perror("Creating pipe error.\n");
        exit(2);
    }

    kids_alive = prac; // ustaw licznik potomkow

    int zrodlo_fd, sukcesy_fd, raporty_fd;
    open_files(&zrodlo_fd, &sukcesy_fd, &raporty_fd);
    initialize_report_headers(raporty_fd);
    initialize_zeros(sukcesy_fd);

    int is_parent = create_first_kids(raporty_fd);

    if (is_parent)
    {
        parent(zrodlo_fd, sukcesy_fd, raporty_fd);
    }
    else
    {
        child();
    }
}

void child()
{

    close(parent_to_child[1]);
    close(child_to_parent[0]);
    // write: child_to_parent, read: parent_to_child
    dup2(parent_to_child[0], STDIN_FILENO);
    dup2(child_to_parent[1], STDOUT_FILENO);

    close(parent_to_child[0]);
    close(child_to_parent[1]);

    char *args[] = {"./poszukiwacz", blok_str, NULL};
    int exec_res = execvp(args[0], args);
    if (exec_res == -1)
    {
        perror("Error while executing ./poszukiwacz");
        exit(1);
    }
}

void parent(int zrodlo_fd, int sukcesy_fd, int raporty_fd)
{

    set_nonblock_mode();

    int sleep_res;
    int record_read;      // do odczytania danych od potomkow
    RECORD record_buffer; // do odczytania rekordu od potomka
    unsigned short data_buffer[DATA_BUFFER_SIZE / 2];
    const struct timespec t = FL2NANOSEC(5.48);

    int processed = 0;
    int data_sent;
    int pipe_open = 1;

    while (kids_alive > 0 /* OR THERE IS STILL STH TO READ ??*/)
    {

        if (processed % DATA_BUFFER_SIZE == 0 && processed < wolumen * 2)
        {
            dprintf(debug_fd, "%s", "READ DATA -------------------------\n");
            read_data(zrodlo_fd, data_buffer);
        }

        if (processed < 2 * wolumen)
        {
            data_sent = send_data(data_buffer + (processed % DATA_BUFFER_SIZE) / 2);
            processed += data_sent;
            dprintf(debug_fd, "SEND DATA: %d PROCESSED: %d\n", data_sent, processed);
        }
        else if (pipe_open == 1)
        {
            printf("PROCESSED: %d, WOLUMEN: %ld, CLOSING PIPE\n", processed, wolumen);
            close(parent_to_child[0]);
            pipe_open = 0;
        }

        record_read = read(child_to_parent[0], &record_buffer, sizeof(RECORD));
        dprintf(debug_fd, "RECORD READ: pid:%d x:%d\n", record_buffer.pid, record_buffer.x);
        if (record_read == sizeof(RECORD))
        {
            save_record(sukcesy_fd, &record_buffer);
        }
        if (handle_deaths(raporty_fd, pipe_open) == -1 && record_read == -1)
        {
            sleep_res = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t, NULL);
            if (sleep_res == -1)
            {
                perror("Nanosleep error.");
            }
        }
    }

    printf("DEATH COUNTER: %d\n", death_counter);      // DEBUG
    printf("BIRTH COUNTER: %d\n", birth_counter);      // DEBUG
    printf("TOTAL SUCCESSES: %d\n", num_of_successes); // DEBUG

    close(child_to_parent[1]);
    //close(parent_to_child[0]);
}

int handle_deaths(int raporty_fd, int pipe_open)
{
    // Funkcja zwraca liczbe zarejestrowanych smierci lub -1 jesli nikt nie zginal.
    int result = -1;
    int status;
    int return_value;
    /* 
    maksymalna liczba komorek w pliku to najwieksza dodatnia liczba 2-bajtowa,
    poniewaz takimi liczbami sa indeksy na ktorych wpisujemy pidy
    zajetosc pliku obliczamy jako stosunek dotychczasowych sukcesow do tej liczby
    */
    double filled_slots = ((double)num_of_successes) / MAX_USHORT;
    pid_t pid;
    int fork_result;
    struct timespec t;
    int clock_res;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        result = result != -1 ? result + 1 : 1;

        if (WIFEXITED(status))
        {
            kids_alive--;
            death_counter++;
            clock_res = clock_gettime(CLOCK_MONOTONIC, &t);
            if (clock_res == -1)
            {
                perror("Error while reading a clock.");
                exit(1);
            }
            write_report(raporty_fd, pid, "dead", &t);

            return_value = WEXITSTATUS(status);
            if (return_value <= 10 && filled_slots < 0.75 && pipe_open)
            {
                fork_result = fork();
                if (fork_result == -1)
                {
                    perror("Fork in signal handler error.");
                    exit(1);
                }
                else if (fork_result == 0)
                {
                    child();
                }
                else
                {
                    kids_alive++;
                    clock_res = clock_gettime(CLOCK_MONOTONIC, &t);
                    if (clock_res == -1)
                    {
                        perror("Error while reading a clock.");
                        exit(1);
                    }
                    write_report(raporty_fd, fork_result, "born", &t);
                    birth_counter++;
                }
            }
        }
    }
    return result;
}

void initialize_report_headers(int raporty_fd)
{
    char *format = "%5s %4s %17s\n";
    int res = dprintf(raporty_fd, format, "PID", "INFO", "CLOCK MONOTONIC");
    if (res < 0)
    {
        perror("Error while writing report.");
        exit(1);
    }
}

void write_report(int raporty_fd, int pid, char *note, struct timespec *t)
{
    dprintf(debug_fd, "%s %d\n", note, pid);
    char *format = "%5d %4s %7ld.%-9ld\n";
    int res = dprintf(raporty_fd, format, pid, note, t->tv_sec, t->tv_nsec);
    if (res < 0)
    {
        perror("Error while writing report.");
        exit(1);
    }
}

void open_files(int *zrodlo_fd, int *sukcesy_fd, int *raporty_fd)
{
    *zrodlo_fd = open(zrodlo, O_RDONLY);
    if (*zrodlo_fd == -1)
    {
        perror("Error while opening file: zrodlo");
        exit(1);
    }

    *sukcesy_fd = open(sukcesy, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if (*sukcesy_fd == -1)
    {
        perror("Error while opening file: sukcesy");
        exit(1);
    }

    *raporty_fd = open(raporty, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IRWXU);
    if (*raporty_fd == -1)
    {
        perror("Error while opening file: raporty");
        exit(1);
    }
}

int create_first_kids(int raporty_fd)
{
    struct timespec t;
    int clock_res;
    int pid;
    for (int i = 0; i < prac; i++)
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
        else
        {
            clock_res = clock_gettime(CLOCK_MONOTONIC, &t);
            if (clock_res == -1)
            {
                perror("Error while reading a clock.");
                exit(1);
            }
            write_report(raporty_fd, pid, "born", &t);
            birth_counter++;
        }
    }

    if (pid == 0)
    {
        return 0; // Jestem potomkiem
    }
    else
    {
        return 1; // Jestem rodzicem
    }
}

void set_nonblock_mode()
{
    int nonblock_res = fcntl(parent_to_child[1], F_SETFL,
                             fcntl(parent_to_child[1], F_GETFL) | O_NONBLOCK);
    if (nonblock_res == -1)
    {
        perror("Error while setting nonblock mode.");
        exit(1);
    }
    nonblock_res = fcntl(child_to_parent[0], F_SETFL,
                         fcntl(child_to_parent[0], F_GETFL) | O_NONBLOCK);
    if (nonblock_res == -1)
    {
        perror("Error while setting nonblock mode.");
        exit(1);
    }
}

void read_data(int zrodlo_fd, unsigned short *data_buffer)
{
    int data_read = read(zrodlo_fd, data_buffer, DATA_BUFFER_SIZE);
    if (data_read == -1)
    {
        printf("Error reading data from source\n");
        //TODO: check value from read
    }
}

int send_data(unsigned short *data_buffer)
{
    int data_sent = write(parent_to_child[1], data_buffer, DATA_BUFFER_SIZE);
    if (data_sent == -1)
    {
        dprintf(debug_fd, "%s\n", "Pipe full");
        //TODO: check value from write
        return 0;
    }
    else
    {
        return data_sent;
    }
}

void read_record()
{
}

void save_record(int sukcesy_fd, RECORD *record_buffer)
{
    dprintf(debug_fd, "SAVE RECORD FUNC\n");
    pid_t pid_buffer;
    int record_written;
    //printf("R - pid: %d x: %d\n", record_buffer.pid, record_buffer.x); // DEBUG
    // przesuniecie na odpowiedni indeks
    off_t index = lseek(sukcesy_fd, record_buffer->x * sizeof(pid_t), SEEK_SET);
    if (index == -1)
    {
        perror("lseek error");
        exit(1);
    }

    /*
            Sprawdzenie czy komorka jest pusta - jesli tak, cofniecie sie na jej
            poczatek i wpisanie pid. Jesli nie, nic nie wpisujemy.
            */

    int slot_read = read(sukcesy_fd, &pid_buffer, sizeof(pid_t));
    if (slot_read == -1)
    {
        perror("Slot read error");
        exit(1);
    }
    dprintf(debug_fd, "SAVE RECORD FUNC: pid_buffer %d\n", pid_buffer);

    if (pid_buffer == 0)
    {
        dprintf(debug_fd, "SAVE RECORD FUNC: pusta\n");
        // komorka jest pusta
        index = lseek(sukcesy_fd, -1 * sizeof(pid_t), SEEK_CUR);
        record_written = write(sukcesy_fd, &record_buffer->pid, sizeof(pid_t));
        if (record_written == -1)
        {
            perror("sukcesy writing error");
            exit(1);
        }
        num_of_successes++; // aktualizowanie zapelnienia pliku sukcesy
    }
    else
    {
        dprintf(debug_fd, "SAVE RECORD FUNC: nie pusta\n");
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
            zrodlo = optarg;
            break;
        case 's':
            wolumen = strtoul(optarg, &end, 10);
            if (*end != '\0')
            {
                invalid_arg = optarg;
            }
            break;
        case 'w':
            blok_str = optarg;
            blok = strtoul(optarg, &end, 10);
            if (*end != '\0')
            {
                invalid_arg = optarg;
            }
            break;
        case 'f':
            sukcesy = optarg;
            break;
        case 'l':
            raporty = optarg;
            break;
        case 'p':
            prac = (unsigned int)strtoul(optarg, &end, 10);
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

void initialize_zeros(int sukcesy_fd)
{
    int res = ftruncate(sukcesy_fd, MAX_USHORT * 2);
    if (res == -1)
    {
        perror("Error while initializing file with zeros.");
        exit(1);
    }
}