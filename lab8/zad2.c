#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

void child();
void parent();

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
    signal(SIGPIPE, SIG_IGN);

    const struct timespec t = FL2NANOSEC(0.4);

    char* message = "Hey\n";

    int res;
    while(1) {
        nanosleep(&t, NULL);
        res = write(fd[1], message, 4);
        if (res == -1 && errno == EPIPE) {
            perror("Pipe is closed");
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