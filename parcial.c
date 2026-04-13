#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define N 3  // cantidad de hijos

// Handler de señales
void handler(int sig) {
    if (sig == SIGUSR1) {
        printf("Soy %d y recibi SIGUSR1\n", getpid());
    } else if (sig == SIGTERM) {
        printf("Soy %d y finaliza\n", getpid());
        exit(0);
    }
}

int main() {
    pid_t hijos[N];

    // Configurar sigaction
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Crear hijos
    for (int i = 0; i < N; i++) {
        hijos[i] = fork();

        if (hijos[i] == 0) {
            // Código del hijo
            printf("Hijo creado con PID %d\n", getpid());

            while (1) {
                pause(); // espera señales
            }
        }
    }

    // Código del padre
    sleep(2); // esperar que hijos estén listos

    printf("\nPadre enviando SIGUSR1...\n");
    for (int i = 0; i < N; i++) {
        kill(hijos[i], SIGUSR1);
    }

    sleep(2);

    printf("\nPadre enviando SIGTERM...\n");
    for (int i = 0; i < N; i++) {
        kill(hijos[i], SIGTERM);
    }

    // Esperar a que terminen los hijos
    for (int i = 0; i < N; i++) {
        waitpid(hijos[i], NULL, 0);
        printf("Hijo %d terminó\n", hijos[i]);
    }

    printf("\nTodos los hijos finalizaron.\n");

    return 0;
}
