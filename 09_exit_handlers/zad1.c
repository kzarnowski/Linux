#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#define NANOSEC 1000000000L
#define FL2NANOSEC(f)   {(long)(f), ((f) - (long)(f))*NANOSEC}


void write_record(int status, void* fd) {
    time_t t = time(0);
    char* current_time = ctime(&t);
    int res = write((int)fd, current_time, 25);
    if (res == -1) {
        perror("Error while writing to file.\n");
    }
}

/*
To close the file, use exit handler instead of closing before return statement in main.
The file must be open when write_record is executed.
*/
void close_file(int status, void* fd) {
    close((int)fd);
}

int main(int argc, char** argv) {
    int fd = open("log.txt", O_RDWR | O_CREAT | O_TRUNC | O_APPEND, S_IRWXU);
    if (fd == -1) {
        perror("Opening error.\n");
        exit(1);
    } else {
        on_exit(close_file, (void*)fd);
    }

    if (argc != 2) {
        fprintf(stderr, "Program requires one argument: unsigned int.\n");
        exit(2);
    }

    const time_t t = time(0);
    char* current_time = ctime(&t);

    write(fd, current_time, 25);

    long n = strtol(argv[1], NULL, 0);
    const struct timespec mid = FL2NANOSEC(0.5); 

    // Register write_record n times
    for (int i = 0; i < n; i++) {
        nanosleep(&mid, NULL);
        on_exit(write_record, (void*)fd);
    }

    return(0);
}