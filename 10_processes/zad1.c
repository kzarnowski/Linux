#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void child();
void parent();

#define NANOSEC 1000000000L
#define FL2NANOSEC(f)   {(long)(f), ((f) - (long)(f))*NANOSEC}

int main() {
    int pid;
    /*
    Jawnie ignorujemy rezultat wykonania zwracany przez potomków, dzięki czemu w systmie nie istnieją procesy zombie,
    tzn. po zakończeniu działania potomka od razu całkowicie umiera. Jeśli nie zrobimy tego jawnie, system domyślnie i tak
    ignoruje ten sygnał SIGCHLD, ale przy domyślnym zachowaniu tworzą się procesy zombie, bo być może rodzic będzie chciał
    w przyszłości odczytać rezultat potomka.

    Eksperyment:
        - połowa potomków czeka 1s
        - druga połowa czeka 1.5s
        - ignorowanie sygnału w rodzicu jest ustawiane po 1.25s

        wtedy połowa powinna być zombie, a połowa od razu zniknąć
    */
    
    signal(SIGCHLD, SIG_IGN);
    for (int i = 0; i < 7; i++) {
        pid = fork();
        
        if (pid == -1) {
            printf("Creating a child error.\n");
        } else if (pid == 0) {
            break;
        }
    }   

    if (pid == 0) {
        child();
    } else {
        parent();
    }
}

void child() {
    const struct timespec t = FL2NANOSEC(1.13);
    nanosleep(&t, NULL);
}

void parent() {
    const struct timespec t = FL2NANOSEC(6.3);
    nanosleep(&t, NULL);
}