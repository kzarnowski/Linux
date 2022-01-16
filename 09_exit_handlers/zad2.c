// ./prog 10 50 200

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#define TIMESTAMP_LEN 25

// struct passed to write_stamp() with on_exit
typedef struct stamp_arg {
    int* fd;
    int offset;
} stamp_arg;

// struct passed to fill_empty() with on_exit
typedef struct fill_arg {
    int* fd;
    int n;
    char c;
} fill_arg;


// Fill the first n bytes of the file with offset given as 
// a command line argument.
void fill_empty(int status, void* p_fill_arg) {
    if (status != EXIT_SUCCESS) {
        return;
    }

    fill_arg* arg = (fill_arg*)p_fill_arg;
    char* s = (char*)malloc(sizeof(char) * arg->n);
    if (s == NULL) {
        perror("Memory error.\n");
        return;
    }

    for (int i = 0; i < arg->n; i++) {
        s[i] = arg->c;
    }
    
    int res = write(*arg->fd, s, arg->n);
    if (res == -1) {
        perror("Writing error.\n");
    }

    free(s);
    free(p_fill_arg);
}

// Write timestamp in the file at given offset.
void write_stamp(int status, void* p_stamp) {
    if (status != EXIT_SUCCESS) {
        return;
    }

    stamp_arg* stamp = (stamp_arg*)p_stamp;

    time_t t = time(0);
    char* current_time = ctime(&t);
    
    lseek(*stamp->fd, stamp->offset, SEEK_SET);
    int res = write(*stamp->fd, current_time, TIMESTAMP_LEN);
    if (res == -1) {
        perror("Writing error.\n");
    }

    free(stamp);
}

// Close file after write_stamp and fill_empty were executed.
void close_file(int status, void* fd) {
    close(*(int*)fd);
}

int main(int argc, char** argv) {
    int* fd = malloc(sizeof(int*));
    // Register file closing here, so it is executed as the last exit_handler.
    on_exit(close_file, (void*)fd);

    int* offs = malloc((argc - 1) * sizeof(int));
    if (offs == NULL) {
        perror("Memory error.\n");
        exit(EXIT_FAILURE);
    }

    int max = 0;
    
    // For every argument, register write_stamp function with given offset.
    for (int i = 0; i < argc - 1; i++) {
        offs[i] = (int)strtol(argv[i+1], NULL, 0);

        if (offs[i] <= 0) {
            fprintf(stderr, "Arguments must be greater than 0.\n");
            exit(EXIT_FAILURE);
        }

        if (offs[i] > max) {
            max = offs[i];
        }

        stamp_arg* stamp = (stamp_arg*)malloc(sizeof(stamp_arg));
        stamp->fd = fd;
        stamp->offset = offs[i];
        on_exit(write_stamp, (void*)stamp);
    }

    free(offs);

    fill_arg* arg = (fill_arg*)malloc(sizeof(fill_arg));
    if (arg == NULL) {
        perror("Memory error.\n");
        exit(EXIT_FAILURE);
    }

    arg->c = '-';
    arg->fd = fd;
    arg->n = max;

    on_exit(fill_empty, (void*)arg);

    *fd = open("log2.txt", O_RDWR | O_CREAT | O_EXCL, S_IRWXU);

    if (*fd == -1) {
        perror("Opening file error.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}