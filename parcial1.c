#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Bandera global para controlar el fin del proceso hijo
volatile sig_atomic_t terminar = 0;

// Manejador de señales para el hijo
void manejador_hijo(int senal) {
    if (senal == SIGUSR1) {
        printf("\n[Hijo %d] Recibí SIGUSR1. ¡Sigo vivo!\n", getpid());
    } else if (senal == SIGTERM) {
        printf("\n[Hijo %d] Recibí SIGTERM. Finalizando...\n", getpid());
        terminar = 1;
    }
}

int main() {
    int n = 2; // Número de hijos a crear
    pid_t pid;

    printf(" supervisor de procesos (PID PADRE: %d) ---\n", getpid());

    for (int i = 0; i < n; i++) {
        pid = fork();

        if (pid < 0) {
            perror("Error en fork");
            exit(1);
        }

        if (pid == 0) {
            
            struct sigaction sa;
            sa.sa_handler = manejador_hijo;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;

            // Configurar handlers
            sigaction(SIGUSR1, &sa, NULL);
            sigaction(SIGTERM, &sa, NULL);

            printf(" Hijo %d creado con PID: %d. Esperando señales...\n", i + 1, getpid());

       
            while (!terminar) {
                pause();
            }
            exit(0);
        }
    }

    
    printf("\nPADRE: Todos los hijos listos. Esperando que terminen (usa otra terminal).\n");
    
    // El padre usa waitpid para monitorear a cada hijo
    for (int i = 0; i < n; i++) {
        int status;
        pid_t hijo_finalizado = waitpid(-1, &status, 0); // Espera a cualquier hijo
        if (WIFEXITED(status)) {
            printf("PADRE: El hijo con PID %d ha finalizado.\n", hijo_finalizado);
        }
    }

    printf("PADRE: Todos los procesos hijos han terminado. Cerrando supervisor.\n");
    return 0;
}
