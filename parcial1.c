#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Handler de señales
void handler(int sig) {
    if (sig == SIGUSR1) {
        printf("Soy %d y recibi SIGUSR1\n", getpid());
    } else if (sig == SIGTERM) {
        printf("Soy %d y finaliza\n", getpid());
        exit(0);
    }
}

int main(int argc, char *argv[]) {

    // Validar argumento
    if (argc != 2) {
        printf("Uso: %s <cantidad_hijos>\n", argv[0]);
        exit(1);
    }

    int N = atoi(argv[1]);
    pid_t *hijos = malloc(N * sizeof(pid_t));

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
            printf("Hijo listo. PID: %d\n", getpid());

            while (1) {
                pause();
            }
        }
    }

    // Padre
    printf("\n=== PADRE ===\n");
    printf("Cantidad de hijos: %d\n", N);

    for (int i = 0; i < N; i++) {
        printf("PID hijo %d: %d\n", i, hijos[i]);
    }

    printf("\nUsa otra terminal para enviar señales:\n");
    printf("kill -SIGUSR1 <PID>\n");
    printf("kill -SIGTERM <PID>\n\n");

    // Esperar hijos
    for (int i = 0; i < N; i++) {
        waitpid(hijos[i], NULL, 0);
        printf("Hijo %d terminó\n", hijos[i]);
    }

    free(hijos);

    printf("Todos los hijos finalizaron\n");
    return 0;
}
