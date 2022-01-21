#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

void child();
void parent();
void handle();

volatile struct timespec t1, t2;

#define NANOSEC 1000000000L
#define FL2NANOSEC(f)   {(long)(f), ((f) - (long)(f))*NANOSEC}

int fd[2];

int main(int argc, char** argv) {
    if (pipe(fd) == -1) {
        perror("Creating pipe error.\n");
        exit(EXIT_FAILURE);
    }

    int pid = fork();

    switch (pid) {
        case -1:
            printf("Creating a child error.\n");
            break;
        case 0:
            child();
            break;
        default:
            parent();
            break;

    }
}

void parent() {
    close(fd[0]);
    signal(SIGPIPE, handle);

    const struct timespec t = FL2NANOSEC(0.6);

    char* message = "Hey\n";

    int res;
    while(1) {
        nanosleep(&t, NULL);
        res = write(fd[1], message, 4);
        if (res == -1 && errno == EPIPE) {
            perror("Pipe is closed");
            struct timespec t3, t4;
            clock_gettime(CLOCK_REALTIME, &t3);
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t4);
            printf("REAL_TIME    (handle): %ld.%ld\n", t1.tv_sec, t1.tv_nsec);
            printf("REAL_TIME    (main):   %ld.%ld\n", t3.tv_sec, t3.tv_nsec);
            printf("PROCESS_TIME (handle): %ld.%ld\n", t2.tv_sec, t2.tv_nsec);
            printf("PROCESS_TIME (main):   %ld.%ld\n", t4.tv_sec, t4.tv_nsec);
            exit(EXIT_SUCCESS);
        }
    }
}

void child() {
    close(fd[1]);

    char buffer[4];
    int res;
    for (int i = 0; i < 5; i++) {
        res = read(fd[0], buffer, sizeof(buffer));
        if (res == -1) {
            break;
        } else {
            printf("%s", buffer);
        }
    }
}

void handle() {
    struct timespec tt1, tt2;
    clock_gettime(CLOCK_REALTIME, &tt1);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tt2);
    t1 = tt1;
    t2 = tt2;
}