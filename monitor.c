#include <stdio.h> 
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// Contador global  
volatile sig_atomic_t contador_global= 0;

// Manejador de señales(2)
void manejador(int senhal) {
    if (senhal == SIGINT) {
        printf("\nRecibido SIGINT (Ctrl+C), pero NO termino.\n");
    } else if (senhal == SIGTERM) {
        printf("\nRecibido SIGTERM. Liberando recursos y terminando...\n");
        exit(0);
    } else if (senhal == SIGUSR1) {
        contador_global++;
        printf("\nRecibido SIGUSR1, Contador = %d\n", contador_global);
    }
}

int main() {

	//Definir la estructura de la configuracion de mi señal(1)
    struct sigaction signhal;

	// pasar la funcion que maneja las señales al sa_handler
    signhal.sa_handler = manejador;
	// sigemptyset no bloquear ninguna señal extra dentro el handler
    sigemptyset(&signhal.sa_mask);
	//ningun comportamiento extra
    signhal.sa_flags = 0;


	//asocia la accion a la señal. Pasa la estructura de configuracion
    sigaction(SIGINT, &signhal, NULL);
    sigaction(SIGTERM, &signhal, NULL);
    sigaction(SIGUSR1, &signhal, NULL);
	
	// SIG_IGN descarta la señal completamente (distinto al bloqueo: no queda pendiente)
    struct sigaction ignorar;
    ignorar.sa_handler = SIG_IGN;
    sigemptyset(&ignorar.sa_mask);
    ignorar.sa_flags = 0;
    sigaction(SIGINT, &ignorar, NULL);
    printf("SIGINT ignorada por 3 segundos (Ctrl+C se descarta)...\n");
    sleep(3);
	// Restauramos el handler original de SIGINT
    sigaction(SIGINT, &signhal, NULL);
    printf("SIGINT restaurada.\n");

    // Esperar señales(4)
    printf("Esperando señales...\n");

    while (1) {
        pause(); // espera hasta recibir una señal
    }

    return 0;
}
