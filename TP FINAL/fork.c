#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid;

    // Aquí el proceso se duplica (se bifurca)
    pid = fork();

    if (pid < 0) {
        // Si fork() devuelve un número negativo, hubo un error
        perror("Error al crear el proceso hijo");
        return 1;
    }
    else if (pid == 0) {
        // Si fork() devuelve 0, significa que estamos dentro del HIJO
        printf("[HIJO] ¡Hola! Mi PID es %d y mi padre es %d.\n", getpid(), getppid());
    }
    else {
        // Si fork() devuelve un número mayor a 0, estamos en el PADRE
        // El valor devuelto es el PID del hijo que acaba de nacer
        printf("[PADRE] Creé a un hijo con PID %d. Mi propio PID es %d.\n", pid, getpid());
    }

    // Este código lo ejecutan AMBOS procesos de forma independiente
    printf("Proceso %d terminando...\n", getpid());

    return 0;
}
