#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

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

char *zrodlo = "./data";
char *sukcesy = "./sukcesy";
char *raporty = "./raporty";
unsigned int prac = 10;
unsigned long int wolumen = 10000;
char *blok_str = "100";
unsigned long int blok = 100;

int parent_to_child[2]; // rodzic wysyla, potomek czyta
int child_to_parent[2]; // rodzic czyta, potomek wysyla

// ------------------------------------------------------------------------- //

void child();
void parent();

// ------------------------------------------------------------------------- //

int main(int argc, char **argv)
{
    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1)
    {
        perror("Creating pipe error.\n");
        exit(2);
    }

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
        printf("CHILD FUNC\n");
        child();
    }
    else
    {
        printf("PARENT FUNC\n");
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
    //int kids_alive = prac;
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

    nanosleep(&t, NULL); // TEST PURPOSE
    while (segment_read != 0 /*data_written || kids_alive*/)
    {
        segment_read = read(child_to_parent[0], &record_buffer, sizeof(RECORD));
        if (segment_read == sizeof(RECORD))
        {
            printf("R - pid: %d x: %d\n", record_buffer.pid, record_buffer.x);
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
    }
}