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

    pid = fork();

    if (pid < 0) {
        perror("Error en fork");
        exit(1);
    }

    if (pid == 0) {
        // HIJO
        printf("Hijo ejecutandose con PID %d\n", getpid());
        printf("Esperando señales...\n");

        while (1) {
            pause(); // espera señales indefinidamente
        }

    } else {
        // PADRE
        printf("Padre PID %d, hijo PID %d\n", getpid(), pid);

        // El padre solo monitorea
        waitpid(pid, NULL, 0);

        printf("Hijo finalizado\n");
    }

    return 0;
}
