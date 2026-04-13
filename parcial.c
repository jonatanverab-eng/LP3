#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

void handler(int sig) {
    if (sig == SIGUSR1) {
        printf("Soy PID %d y recibi SIGUSR1\n", getpid());
    } else if (sig == SIGTERM) {
        printf("Soy PID %d y finalizo\n", getpid());
        exit(0);
    }
}

int main() {
    pid_t pid;
    struct sigaction sa;

    // Configurar handler
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Crear hijo
    pid = fork();

    if (pid < 0) {
        perror("Error en fork");
        exit(1);
    }

    if (pid == 0) {
        // Código del hijo
        printf("Hijo creado con PID %d\n", getpid());

        while (1) {
            pause(); // Espera señales
        }
    } else {
        // Código del padre
        sleep(1);

        printf("Padre envia SIGUSR1 al hijo\n");
        kill(pid, SIGUSR1);

        sleep(1);

        printf("Padre envia SIGTERM al hijo\n");
        kill(pid, SIGTERM);

        // Monitorear finalización del hijo
        waitpid(pid, NULL, 0);
        printf("Hijo finalizado\n");
    }

    return 0;
}
