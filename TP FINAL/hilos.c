#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

// --- CONFIGURACIÓN DURA ---
#define CANTIDAD_HILOS 10  // Podés cambiar este número a 50 o lo que quieras

// Mutex para proteger la zona de trabajo
pthread_mutex_t mutex_trabajo = PTHREAD_MUTEX_INITIALIZER;

// Función que ejecutarán todos los hilos
void* funcion_trabajar(void* arg) {
    long id = (long)arg; // Recuperamos el número del hilo

    printf("[Hilo %ld] -> ¡Acabo de nacer y quiero entrar a trabajar!\n", id);

    // Intentamos agarrar el mutex. Si está ocupado (EBUSY), avisamos que estamos esperando
    while (pthread_mutex_trylock(&mutex_trabajo) == EBUSY) {
        printf("[Hilo %ld]   ... está ocupado, sigo esperando afuera...\n", id);
        
        // Espera un poquito (en microsegundos) antes de volver a preguntar, 
        // así da tiempo a ver los mensajes en orden.
        usleep(300000); 
    }

    // --- ¡AQUÍ ENTRÓ A LA FUNCIÓN (ZONA CRÍTICA)! ---
    // Ningún otro hilo puede estar en este bloque de código al mismo tiempo
    printf("\n==================================================\n");
    printf("[Hilo %ld] >>> ¡ENTRÉ! Ahora YO estoy trabajando en la función.\n", id);
    printf("==================================================\n\n");

    // Simulamos que el hilo está haciendo un trabajo pesado durmiendo un breve momento
    usleep(600000); 

    printf("[Hilo %ld] <<< Terminé mi trabajo. Saliendo y liberando el mutex...\n", id);
    
    // Soltamos el mutex para que el siguiente hilo en la fila pueda entrar
    pthread_mutex_unlock(&mutex_trabajo);

    pthread_exit(NULL);
}

int main() {
    // Creamos un array de hilos usando la constante definida arriba
    pthread_t hilos[CANTIDAD_HILOS];

    printf("[MAIN] Iniciando el programa. Vamos a crear %d hilos...\n", CANTIDAD_HILOS);

    // Creación de los hilos en un bucle
    for (long i = 0; i < CANTIDAD_HILOS; i++) {
        // Pasamos "i+1" como el ID del hilo para que se lea "Hilo 1", "Hilo 2", etc.
        if (pthread_create(&hilos[i], NULL, funcion_trabajar, (void*)(i + 1)) != 0) {
            perror("Error al crear el hilo");
            return 1;
        }
    }

    printf("[MAIN] Todos los hilos fueron creados. Esperando que terminen con JOIN...\n");

    // El Main espera obligatoriamente que terminen todos los hilos antes de cerrar el programa
    for (int i = 0; i < CANTIDAD_HILOS; i++) {
        pthread_join(hilos[i], NULL);
    }

    printf("[MAIN] ¡Excelente! Todos los %d hilos terminaron de trabajar. Fin del programa.\n", CANTIDAD_HILOS);

    // Destruimos el mutex al limpiar el programa
    pthread_mutex_destroy(&mutex_trabajo);

    return 0;
}
