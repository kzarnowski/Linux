#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#define NANOSEC 1000000000L
#define FL2NANOSEC(f)   {(long)(f), ((f) - (long)(f))*NANOSEC}

int main(int argc, char** argv) {
    int fd = open("file", O_RDWR | O_CREAT | S_IRWXU);
    if (fd == -1) {
        perror("File opening error.\n");
        exit(EXIT_FAILURE);
    }

    int pid;
    int res;
    char* message;
    switch (pid = fork()) {
        case -1:
            perror("Fork error.\n");
            exit(EXIT_FAILURE);
        case 0:
            message = "Child\n";
            res = write(fd, message, 6);
            break;
        default:
            message = "Parent\n";
            res = write(fd, message, 7);
            break;
    }

    if (res == -1) {
        perror("Writing error.");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        int status;
        wait(&status);
        char buffer[13];

        res = read(fd, buffer, 13);
        if (res == -1) {
            perror("Reading error.");
            exit(EXIT_FAILURE);
        }
        printf("%s\n", buffer);
    }
    return 0;
}