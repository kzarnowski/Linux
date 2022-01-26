#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>

#define N 10

typedef struct clocks {
    struct timespec t1;
    struct timespec t2;
} clocks;

volatile typedef struct queue {
    sig_atomic_t first;
    sig_atomic_t  last;
    clocks times[N];
} queue;

queue q;
#define NANOSEC 1000000000L
#define FL2NANOSEC(f)   {(long)(f), ((f) - (long)(f))*NANOSEC}

int isEmpty();
void handle();
int push(clocks tt);
clocks pop();

int main() {
    signal(SIGRTMIN, handle);
    signal(SIGUSR1, handle);
    signal(SIGUSR2, handle);

    const struct timespec t = FL2NANOSEC(1.5);
    clocks temp;
    int res;
    while(1) {
        while (!isEmpty()) {
            temp = pop();
            printf("REAL: %ld.%ld\nPROCESS:%ld.%ld\n", 
                temp.t1.tv_sec, temp.t1.tv_nsec,
                temp.t2.tv_sec, temp.t2.tv_nsec);
        }
        res = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t, NULL);
        if (res == -1) {
            perror("Nanosleep error.");
        }
    }
}

void handle() {
    struct timespec tt1, tt2;
    clock_gettime(CLOCK_REALTIME, &tt1);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tt2);
    clocks times = {tt1, tt2};
    push(times);
}

int push(clocks tt) {
    if ((q.last + 1) % N == q.first) {
        // queue is full
        return -1;
    }

    q.last = (q.last + 1) % N;
    q.times[q.last] = tt;
    return 0;
}

clocks pop() {
    q.first = (q.first + 1) % N;
    return q.times[q.first - 1];
}

int isEmpty() {
    return q.first == q.last;
}