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

#define DATA_BUFFER_LEN 1000

// ------------------------------------------------------------------------- //

char *zrodlo = "./data_3";
char *sukcesy = "./sukcesy";
char *raporty = "./raporty";
unsigned int prac = 1;
unsigned long int wolumen = 10000;
char *blok_str = "100";
unsigned long int blok = 100;

int parent_to_child[2]; // rodzic wysyla, potomek czyta
int child_to_parent[2]; // rodzic czyta, potomek wysyla

volatile int death_counter = 0;
volatile int kids_alive;

// ------------------------------------------------------------------------- //

void child();
void parent();
void signal_register(int signum, void *func, struct sigaction *sa);
void handle_deaths();

// ------------------------------------------------------------------------- //

int main(int argc, char **argv)
{
    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1)
    {
        perror("Creating pipe error.\n");
        exit(2);
    }

    kids_alive = prac; // ustaw licznik potomkow

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

void parent()
{
    struct sigaction sa;
    signal_register(SIGCHLD, handle_deaths, &sa);

    close(parent_to_child[0]);
    close(child_to_parent[1]);

    const struct timespec t = FL2NANOSEC(0.48);

    // zmiana na tryb nieblokujacy
    fcntl(parent_to_child[1], F_SETFL, fcntl(parent_to_child[1], F_GETFL) | O_NONBLOCK); //TODO: add error checking
    fcntl(child_to_parent[0], F_SETFL, fcntl(child_to_parent[0], F_GETFL) | O_NONBLOCK); //TODO: add error checking

    int zrodlo_fd = open(zrodlo, O_RDONLY);
    int sukcesy_fd = open(sukcesy, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    //int raporty_fd = open(raporty, O_RDWR | O_CREAT | O_TRUNC,  S_IRWXU);

    unsigned short data_buffer[DATA_BUFFER_LEN];

    int data_read;
    int data_written;
    int segment_read = 1;
    int segment_written;
    RECORD record_buffer;

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

    nanosleep(&t, NULL); // DEBUG
    int i = 0;
    while (kids_alive /*data_written || kids_alive*/)
    {
        segment_read = read(child_to_parent[0], &record_buffer, sizeof(RECORD));
        if (segment_read == sizeof(RECORD))
        {
            printf("R - pid: %d x: %d\n", record_buffer.pid, record_buffer.x); // DEBUG
            off_t index = lseek(sukcesy_fd, record_buffer.x * sizeof(pid_t), SEEK_SET);
            if (index == -1)
            {
                perror("lseek error");
                exit(1);
            }
            segment_written = write(sukcesy_fd, &record_buffer.pid, sizeof(pid_t));
            if (segment_written == -1)
            {
                perror("sukcesy writing error");
                exit(1);
            }
        }
        i++;
    }

    printf("DEATH COUNTER: %d\n", death_counter); // DEBUG
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

void handle_deaths(int signo, siginfo_t *SI, void *data)
{
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        kids_alive--;
        death_counter++;
        printf("child %u terminated.\n", (unsigned)pid);
    }
}