#include <unistd.h>
#include <time.h>
#include <stdio.h>

#define NANOSEC 1000000000L
#define FL2NANOSEC(f)   {(long)(f), ((f) - (long)(f))*NANOSEC}

int main() {
    struct timespec sleep_tm = FL2NANOSEC(0.3);
    printf("Witam");
    nanosleep(&sleep_tm, NULL);
    pid_t n = fork();
    switch (n) {
        case -1:
            return 1;
        case 0:
            printf(", jestem potomkiem");
            nanosleep(&sleep_tm, NULL);
            printf(" rodzica pid=%d", getppid());
            break;
        default:
            printf(", jestem rodzicem");
            nanosleep(&sleep_tm, NULL);
            printf(" procesu pid=%d", n);
    }

    //printf(", jestem procesem");
    nanosleep(&sleep_tm, NULL);
    printf(" (%d)\n", getpid());
    //printf("sec: %ld, nanosec: %ld\n", sleep_tm.tv_sec, sleep_tm.tv_nsec);
}