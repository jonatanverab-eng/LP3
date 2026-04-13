#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Bandera para controlar el bucle del hijo
volatile sig_atomic_t continuar = 1;

// Manejador de señales para el proceso hijo
void manejador_hijo(int senal) {
    if (senal == SIGUSR1) {
        printf("Soy el PID %d y recibí SIGUSR1\n", getpid());
    } else if (senal == SIGTERM) {
        printf("Soy el PID %d y finalizo\n", getpid());
        continuar = 0;
    }
}

int main() {
    int num_hijos = 2; // Puedes cambiar el número de hijos a crear
    pid_t pids[num_hijos];

    for (int i = 0; i < num_hijos; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("Error al crear el proceso");
            exit(1);
        }

        if (pids[i] == 0) {
            // --- CÓDIGO DEL HIJO ---
            struct sigaction sa;
            sa.sa_handler = manejador_hijo;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;

            // Implementar handlers con sigaction()
            sigaction(SIGUSR1, &sa, NULL);
            sigaction(SIGTERM, &sa, NULL);

            printf("Hijo %d (PID: %d) esperando señales...\n", i + 1, getpid());

            while (continuar) {
                pause(); // Espera a que llegue una señal
            }
            exit(0);
        }
    }

    sleep(1);

    for (int i = 0; i < num_hijos; i++) {
        printf("\nPadre enviando SIGUSR1 al hijo %d...\n", pids[i]);
        kill(pids[i], SIGUSR1);
        sleep(1);

        printf("Padre enviando SIGTERM al hijo %d...\n", pids[i]);
        kill(pids[i], SIGTERM);

        // Monitoreo: Usa waitpid() para esperar la finalización
        int status;
        waitpid(pids[i], &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Padre: El hijo %d terminó correctamente.\n", pids[i]);
        }
    }

    printf("\nPadre: Todos los hijos han finalizado. Terminando supervisor.\n");
    return 0;
}
