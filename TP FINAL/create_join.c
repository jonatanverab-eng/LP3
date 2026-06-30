#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> // Librería fundamental para hilos

// Función que ejecutará el hilo
void* funcion_hilo(void* arg) {
    int id = *(int*)arg;
    printf("[HILO %d] ¡Hola desde el hilo secundario!\n", id);
    
    // pthread_exit se usa para terminar el hilo limpiamente y devolver un valor
    int* resultado = malloc(sizeof(int));
    *resultado = id * 100;
    pthread_exit(resultado); 
}

int main() {
    pthread_t mi_hilo; // Identificador del hilo
    int id = 1;
    int* retorno_hilo;

    printf("[MAIN] Creando el hilo...\n");
    
    // 1. pthread_create: Crea y arranca el hilo
    // Argumentos: (&id_hilo, atributos, función_a_ejecutar, argumento_para_la_función)
    if (pthread_create(&mi_hilo, NULL, funcion_hilo, &id) != 0) {
        perror("Error al crear el hilo");
        return 1;
    }

    printf("[MAIN] El hilo está corriendo. Ahora voy a esperar a que termine...\n");

    // 2. pthread_join: Bloquea al main hasta que el hilo especificado finalice
    // Es el equivalente al waitpid() de los procesos.
    if (pthread_join(mi_hilo, (void**)&retorno_hilo) != 0) {
        perror("Error al esperar al hilo");
        return 1;
    }

    printf("[MAIN] El hilo terminó. Nos devolvió el valor: %d\n", *retorno_hilo);
    
    free(retorno_hilo); // Limpiar memoria
    return 0;
}
