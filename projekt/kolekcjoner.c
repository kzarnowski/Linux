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
#include <errno.h>

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

#define DATA_BUFFER_SIZE 512
#define MAX_USHORT 65535 // 2^16 - 1

// ------------------------------------------------------------------------- //

char *zrodlo;              // -d <zrodlo>    dane do pobrania
char *sukcesy;             // -f <sukcesy>   sciezka do pliku osiagniec
char *raporty;             // -l <raporty>   sciezka do pliku raportow
unsigned long int wolumen; // -s <wolumen>   ilosc danych
unsigned long int blok;    // -w <blok>      ilosc danych na potomka
unsigned int prac;         // -p <prac>      maksymalna liczba potomkow
char *blok_str;            // do przeslania parametru blok do potomka

int parent_to_child[2]; // rodzic wysyla, potomek czyta
int child_to_parent[2]; // rodzic czyta, potomek wysyla

int kids_alive;
int num_of_successes = 0;

int death_counter = 0;
int birth_counter = 0;

// ------------------------------------------------------------------------- //

void child();
void parent(int zrodlo_fd, int sukcesy_fd, int raporty_fd);
void signal_register(int signum, void *func, struct sigaction *sa);
void write_report(int raporty_fd, int pid, char *note, struct timespec *t);
void initialize_report_headers(int raporty_fd);
int handle_deaths(int raporty_fd, int pipe_open);
void open_files(int *zrodlo_fd, int *sukcesy_fd, int *raporty_fd);
void close_files(int zrodlo_fd, int sukcesy_fd, int raporty_fd);
void close_fd(int fd);
int create_first_kids(int raporty_fd);
void set_nonblock_mode();
int read_args(int argc, char **argv);
void initialize_zeros(int sukcesy_fd);
unsigned long int calculate_unit(unsigned long int x, char *end);

int read_data(int zrodlo_fd, unsigned short *data_buffer, long int total_read);
//int send_data(unsigned short *data_buffer, long int processed, int data_read);
int send_data(unsigned short *data_buffer, long int processed);
void read_record();
void save_record(int sukcesy_fd, RECORD *record_buffer);

// ------------------------------------------------------------------------- //

int main(int argc, char **argv)
{
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

    kids_alive = prac; // ustawienie licznika potomkow

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

    close_fd(parent_to_child[0]);
    close_fd(child_to_parent[1]);

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
    const struct timespec t = FL2NANOSEC(0.48);

    long int processed = 0;  // liczba dotychczas wyslanych bajtow
    int data_sent;           // liczba bajtow wyslanych w danym przebiegu petli
    int data_read;           // liczba bajtow odczytanych ze zrodla w danym przebiegu
    int pipe_open = 1;       // stan parent_to_child[0]: 1 - otwarty, 0 - zamkniety
    long int total_read = 0; // liczba wszystkich wczytanych bajtow

    // Petla dziala dopoki choc jeden potomek zyje, lub rodzic cos czyta z pipe'a
    while (kids_alive > 0 || record_read > 0)
    {
        /*
            Poniewaz wysylamy na raz duza porcje danych, a odczytujemy po jednym
            rekordzie, chcemy czesciej odczytywac niz wysylac. 
            W tym celu zliczamy liczbe wyslanych bajtow i nowa porcje
            danych ze zrodla wczytujemy z krokiem modulo rozmiar bufora.
        */

        // Wczytywanie nowej porcji danych, kiedy caly data_buffer zostal wyslany
        if (processed % DATA_BUFFER_SIZE == 0 && /*processed < wolumen * 2*/ total_read < 2 * wolumen)
        {
            data_read = read_data(zrodlo_fd, data_buffer, total_read);
            total_read += data_read;
            printf("TOTAL READ: %ld, DATA READ: %d\n", total_read, data_read);
        }

        if (processed < 2 * wolumen)
        {
            data_sent = send_data(data_buffer + (processed % DATA_BUFFER_SIZE) / 2, processed);
            processed += data_sent;
            printf("TOTAL SENT: %ld, DATA SENT: %d\n", processed, data_sent);
        }
        else if (pipe_open == 1)
        {
            // wyslano wszystkie dane
            close_fd(parent_to_child[1]);
            pipe_open = 0;
        }

        // wczytanie rekordu od potomka i zapisanie do pliku sukcesow
        record_read = read(child_to_parent[0], &record_buffer, sizeof(RECORD));
        if (record_read == -1 && errno != EAGAIN)
        {
            perror("record read error");
            exit(1);
        }

        if (record_read == sizeof(RECORD))
        {
            save_record(sukcesy_fd, &record_buffer);
        }

        // warunek jest spelniony jesli w danym przebiegu zaden z potomkow
        // nie umarl i jednoczesnie nic nie odczytano
        if (handle_deaths(raporty_fd, pipe_open) == -1 && record_read == -1)
        {
            sleep_res = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t, NULL);
            if (sleep_res == -1)
            {
                perror("Nanosleep error.");
            }
        }
    }

    close_fd(child_to_parent[0]);
    close_files(zrodlo_fd, sukcesy_fd, raporty_fd);

    printf("DEATH COUNTER: %d\n", death_counter); // DEBUG
    printf("BIRTH COUNTER: %d\n", birth_counter); // DEBUG
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

    /*
    obsluga funkcji waitpid na podstawie:
    https://www.ibm.com/docs/en/zos/2.4.0?topic=functions-waitpid-wait-specific-child-process-end
    https://stackoverflow.com/questions/11322488/how-to-make-sure-that-waitpid-1-stat-wnohang-collect-all-children-process
    */

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        result = result != -1 ? result + 1 : 1;

        if (WIFEXITED(status))
        {
            death_counter++;
            kids_alive--;
            clock_res = clock_gettime(CLOCK_MONOTONIC, &t);
            if (clock_res == -1)
            {
                perror("Error while reading a clock.");
                exit(1);
            }
            write_report(raporty_fd, pid, "dead", &t);

            return_value = WEXITSTATUS(status);

            // nowych potomkow tworzymy jesli ten ktory zginal nie zakonczyl
            // sie bledem i wypenienie pliku sukcesow nie przekroczylo 75%
            if (return_value <= 10 && filled_slots < 0.75)
            {
                fork_result = fork();
                if (fork_result == -1)
                {
                    perror("Fork in deaths handler error.");
                    exit(1);
                }
                else if (fork_result == 0)
                {
                    child();
                }
                else
                {
                    // proces rodzica, rejestracja utworzenia potomka
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
    // na podstawie man printf oraz
    // https://stackoverflow.com/questions/8304259/formatting-struct-timespec
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
            // proces rodzica, rejestracja utworzenia potomka
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
    // na podstawie https://www.linuxtoday.com/blog/blocking-and-non-blocking-i-0/
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

int read_data(int zrodlo_fd, unsigned short *data_buffer, long int total_read)
{
    int data_to_read = DATA_BUFFER_SIZE;
    if (wolumen * 2 - total_read < DATA_BUFFER_SIZE)
    {
        // w ostatnim pakiecie czytamy mniej
        data_to_read = wolumen * 2 - total_read;
    }
    int data_read = read(zrodlo_fd, data_buffer, data_to_read);
    if (data_read == -1)
    {
        perror("Error reading data from source\n");
        exit(1);
    }
    return data_read;
}

int send_data(unsigned short *data_buffer, long int processed)
{
    int data_to_send = DATA_BUFFER_SIZE - (processed % DATA_BUFFER_SIZE);
    if (wolumen * 2 - processed < DATA_BUFFER_SIZE)
    {
        // w ostatnim pakiecie wysylamy mniej
        data_to_send = wolumen * 2 - processed;
    }

    int data_sent = write(parent_to_child[1], data_buffer, data_to_send);
    if (data_sent == -1 && errno == EAGAIN)
    {
        return 0;
    }
    else if (data_sent == -1 && errno != EAGAIN)
    {
        perror("Error while writing parent to child");
        exit(1);
    }
    else
    {
        return data_sent;
    }
}

void save_record(int sukcesy_fd, RECORD *record_buffer)
{
    pid_t pid_buffer;
    int record_written;

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

    if (pid_buffer == 0)
    {
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
}

int read_args(int argc, char **argv)
{
    /*
    Sprawdzanie case '?' oraz optind < argc zaczerpnięte z
    The GNU C Library Reference Manual 25.2.2 Example of Parsing Arguments with getopt
    */
    int option;
    char *end;

    while ((option = getopt(argc, argv, "d:s:w:f:l:p:")) != -1)
    {
        switch (option)
        {
        case 'd':
            zrodlo = optarg;
            break;
        case 's':
            wolumen = strtoul(optarg, &end, 10);
            wolumen = calculate_unit(wolumen, end);
            break;
        case 'w':
            blok_str = optarg;
            blok = strtoul(optarg, &end, 10);
            blok = calculate_unit(blok, end);
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
                fprintf(stderr, "Invalid argument: prac\n");
                return 1;
            }
            break;
        case '?':
            return 1;
        }
    }

    if (optind < argc)
    {
        fprintf(stderr, "Too many arguments. ");
        return 1;
    }

    return 0;
}

unsigned long int calculate_unit(unsigned long int x, char *end)
{
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
            exit(1);
        }
    }
    return x * multiplier;
}

void initialize_zeros(int sukcesy_fd)
{
    // zrodlo: man 2 truncate
    int res = ftruncate(sukcesy_fd, MAX_USHORT * 2);
    if (res == -1)
    {
        perror("Error while initializing file with zeros.");
        exit(1);
    }
}

void close_files(int zrodlo_fd, int sukcesy_fd, int raporty_fd)
{
    int res;

    res = close(zrodlo_fd);
    if (res == -1)
    {
        perror("Error while closing file: zrodlo");
        exit(1);
    }

    res = close(sukcesy_fd);
    if (res == -1)
    {
        perror("Error while closing file: sukcesy");
        exit(1);
    }

    res = close(raporty_fd);
    if (res == -1)
    {
        perror("Error while closing file: raporty");
        exit(1);
    }
}

void close_fd(int fd)
{
    if (close(fd) == -1)
    {
        perror("Error while closing pipe");
        exit(1);
    }
}