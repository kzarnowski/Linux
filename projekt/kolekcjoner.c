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

#define DATA_BUFFER_LEN 10000
#define MAX_USHORT 65535 // 2^16 - 1

// ------------------------------------------------------------------------- //

char *zrodlo = "./data_12";
char *sukcesy = "./sukcesy";
char *raporty = "./raporty";
unsigned int prac = 10;
unsigned long int wolumen = 100000;
char *blok_str = "100";
unsigned long int blok = 100;

int parent_to_child[2]; // rodzic wysyla, potomek czyta
int child_to_parent[2]; // rodzic czyta, potomek wysyla

volatile int death_counter = 0;
volatile int kids_alive;
volatile int num_of_successes = 0;

// ------------------------------------------------------------------------- //

void child();
void parent(int zrodlo_fd, int sukcesy_fd, int raporty_fd);
void signal_register(int signum, void *func, struct sigaction *sa);
void write_report(int raporty_fd, int pid, char *note, struct timespec *t);
void initialize_report_headers(int raporty_fd);
int handle_deaths(int raporty_fd);

// ------------------------------------------------------------------------- //

int main(int argc, char **argv)
{
    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1)
    {
        perror("Creating pipe error.\n");
        exit(2);
    }

    kids_alive = prac; // ustaw licznik potomkow
    struct timespec t;
    int clock_res;

    int zrodlo_fd = open(zrodlo, O_RDONLY);
    if (zrodlo_fd == -1)
    {
        perror("Error while opening file: zrodlo");
        exit(1);
    }
    int sukcesy_fd = open(sukcesy, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    if (sukcesy_fd == -1)
    {
        perror("Error while opening file: sukcesy");
        exit(1);
    }
    int raporty_fd = open(raporty, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, S_IRWXU);
    if (raporty_fd == -1)
    {
        perror("Error while opening file: raporty");
        exit(1);
    }
    initialize_report_headers(raporty_fd);

    int pid;
    for (int i = 0; i < prac /*childs_num*/; i++)
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
        }
    }

    if (pid == 0)
    {
        child();
    }
    else
    {
        parent(zrodlo_fd, sukcesy_fd, raporty_fd);
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
    execvp(args[0], args);

    printf("EXEC error\n");
}

void parent(int zrodlo_fd, int sukcesy_fd, int raporty_fd)
{
    // struct sigaction sa;
    // signal_register(SIGCHLD, handle_deaths, &sa);

    // close(parent_to_child[0]);
    // close(child_to_parent[1]);

    //const struct timespec t = FL2NANOSEC(0.48);

    // zmiana na tryb nieblokujacy
    fcntl(parent_to_child[1], F_SETFL, fcntl(parent_to_child[1], F_GETFL) | O_NONBLOCK); //TODO: add error checking
    fcntl(child_to_parent[0], F_SETFL, fcntl(child_to_parent[0], F_GETFL) | O_NONBLOCK); //TODO: add error checking

    unsigned short data_buffer[DATA_BUFFER_LEN];

    //int sleep_res;
    int slot_read;        // do odczytania komorki z pliku sukcesy
    int data_read;        // do odczytania danych z pliku zrodlo
    int data_written;     // do zapisu danych z data_buffer do pipe'a
    int segment_read = 1; // do odczytania danych od potomkow
    int segment_written;  // do zapisania rekordu w pliku sukcesy
    RECORD record_buffer; // do odczytania rekordu od potomka
    pid_t pid_buffer;     // do odczytania pidu z komorki pliku sukcesy
    off_t index;          // do przechodzenia po pliku

    data_read = read(zrodlo_fd, data_buffer, DATA_BUFFER_LEN * sizeof(unsigned short));
    if (data_read == -1)
    {
        printf("Data has ended\n");
    }
    data_written = write(parent_to_child[1], data_buffer, DATA_BUFFER_LEN * sizeof(unsigned short));
    if (data_written == -1)
    {
        printf("Error writing to pipe\n");
    }

    while (kids_alive /*data_written || kids_alive*/)
    {
        segment_read = read(child_to_parent[0], &record_buffer, sizeof(RECORD));
        if (segment_read == sizeof(RECORD))
        {
            //printf("R - pid: %d x: %d\n", record_buffer.pid, record_buffer.x); // DEBUG
            // przesuniecie na odpowiedni indeks
            index = lseek(sukcesy_fd, record_buffer.x * sizeof(pid_t), SEEK_SET);
            if (index == -1)
            {
                perror("lseek error");
                exit(1);
            }

            /*
            Sprawdzenie czy komorka jest pusta - jesli tak, cofniecie sie na jej
            poczatek i wpisanie pid. Jesli nie, nic nie wpisujemy.
            */
            // if (res == -1) {
            //     perror("Nanosleep error.");
            // }
            slot_read = read(sukcesy_fd, &pid_buffer, sizeof(pid_t));
            if (slot_read == -1)
            {
                perror("slot_read error");
                exit(1);
            }
            if (pid_buffer == 0)
            {
                // komorka jest pusta
                index = lseek(sukcesy_fd, -1 * sizeof(pid_t), SEEK_CUR);
                segment_written = write(sukcesy_fd, &record_buffer.pid, sizeof(pid_t));
                if (segment_written == -1)
                {
                    perror("sukcesy writing error");
                    exit(1);
                }
                num_of_successes++; // aktualizowanie zapelnienia pliku sukcesy
            }
        }
        // if (handle_deaths() == -1)
        // {
        //     sleep_res = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t, NULL);
        //     if (sleep_res == -1) {
        //          perror("Nanosleep error.");
        //     }
        // }
    }

    printf("DEATH COUNTER: %d\n", death_counter);      // DEBUG
    printf("TOTAL SUCCESSES: %d\n", num_of_successes); // DEBUG
}

void signal_register(int signum, void *func, struct sigaction *sa)
{
    sa->sa_sigaction = func;
    sigemptyset(&sa->sa_mask);
    sa->sa_flags = SA_SIGINFO | SA_NOCLDSTOP;

    if (sigaction(signum, sa, NULL) == -1)
    {
        perror("Sigaction error.");
        exit(1);
    }
}

int handle_deaths(int raporty_fd)
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

        kids_alive--;
        death_counter++;

        result = result != -1 ? result + 1 : 1;

        if (WIFEXITED(status))
        {
            clock_res = clock_gettime(CLOCK_MONOTONIC, &t);
            if (clock_res == -1)
            {
                perror("Error while reading a clock.");
                exit(1);
            }
            write_report(raporty_fd, pid, "dead", &t);

            // printf("child %u terminated, status: %d\n", (unsigned)pid, WEXITSTATUS(status)); //DEBUG
            return_value = WEXITSTATUS(status);
            if (return_value <= 10 && filled_slots < 0.75)
            {
                kids_alive++;
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
    char *format = "%5d %4s %7ld.%-9ld\n";
    int res = dprintf(raporty_fd, format, pid, note, t->tv_sec, t->tv_nsec);
    if (res < 0)
    {
        perror("Error while writing report.");
        exit(1);
    }
}