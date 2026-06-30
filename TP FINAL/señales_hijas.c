#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

// Variable global para controlar el bucle del hijo
volatile sig_atomic_t continuar = 1;

// Manejador de señales para los procesos hijos
void manejador_hijo(int senal) {
    pid_t pid = getpid();
    
    if (senal == SIGUSR1) {
        // Nota: printf no es estrictamente async-signal-safe, 
        // pero se usa aquí para cumplir con el formato solicitado en clase.
        printf("soy %d y recibí SIGUSR1\n", pid);
        fflush(stdout);
    } 
    else if (senal == SIGTERM) {
        printf("soy %d y finaliza\n", pid);
        fflush(stdout);
        continuar = 0; // Rompe el bucle para que el hijo termine limpiamente
    }
}

int main() {
    int num_hijos = 2; // Definimos 2 hijos para el ejemplo
    pid_t pids[num_hijos];

    printf("[PADRE] Mi PID es %d. Creando %d hijos...\n", getpid(), num_hijos);

    for (int i = 0; i < num_hijos; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("Error al bifurcar (fork)");
            exit(EXIT_FAILURE);
        }

        // --- CÓDIGO DEL HIJO ---
        if (pids[i] == 0) {
            struct sigaction sa;
            sa.sa_handler = manejador_hijo;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;

            // Configurar el manejador para SIGUSR1 y SIGTERM usando sigaction()
            if (sigaction(SIGUSR1, &sa, NULL) < 0) {
                perror("Error configurando SIGUSR1");
                exit(EXIT_FAILURE);
            }
            if (sigaction(SIGTERM, &sa, NULL) < 0) {
                perror("Error configurando SIGTERM");
                exit(EXIT_FAILURE);
            }

            // El hijo se queda esperando señales en un bucle
            while (continuar) {
                pause(); // Suspende el proceso hasta que llegue una señal
            }

            exit(EXIT_SUCCESS); // El hijo termina
        }
    }

    // --- CÓDIGO DEL PADRE ---
    // Pequeña pausa para asegurar que los hijos configuraron sus manejadores
    sleep(1); 

    // Demostración: El padre envía señales a los hijos
    for (int i = 0; i < num_hijos; i++) {
        printf("[PADRE] Enviando SIGUSR1 al hijo %d\n", pids[i]);
        kill(pids[i], SIGUSR1);
        sleep(1);
    }

    for (int i = 0; i < num_hijos; i++) {
        printf("[PADRE] Enviando SIGTERM al hijo %d\n", pids[i]);
        kill(pids[i], SIGTERM);
    }

    // Monitoreo de los hijos con waitpid()
    printf("[PADRE] Monitoreando la finalización de los hijos...\n");
    for (int i = 0; i < num_hijos; i++) {
        int estado;
        pid_t pid_terminado = waitpid(pids[i], &estado, 0);
        
        if (pid_terminado > 0) {
            if (WIFEXITED(estado)) {
                printf("[PADRE] Hijo con PID %d terminó correctamente (status: %d)\n", 
                       pid_terminado, WEXITSTATUS(estado));
            } else if (WIFSIGNALED(estado)) {
                printf("[PADRE] Hijo con PID %d fue terminado por una señal (%d)\n", 
                       pid_terminado, WTERMSIG(estado));
            }
        }
    }

    printf("[PADRE] Todos los hijos han finalizado. Terminando supervisor.\n");
    return 0;
}
