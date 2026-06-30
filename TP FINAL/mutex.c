#include <stdio.h>
#include <pthread.h>

int contador = 0; // Variable compartida por los hilos
pthread_mutex_init // Se puede inicializar de forma estática así:
pthread_mutex_t cerrojo = PTHREAD_MUTEX_INITIALIZER; 

void* incrementar_contador(void* arg) {
    for (int i = 0; i < 100000; i++) {
        // pthread_mutex_lock: Cierra el cerrojo. 
        // Si otro hilo ya lo cerró, este hilo se duerme aquí hasta que se libere.
        pthread_mutex_lock(&cerrojo);
        
        // --- INICIO DE LA ZONA CRÍTICA ---
        contador++; 
        // --- FIN DE LA ZONA CRÍTICA ---
        
        // pthread_mutex_unlock: Abre el cerrojo para que otros hilos puedan entrar
        pthread_mutex_unlock(&cerrojo);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t hilo1, hilo2;

    // Crear dos hilos que van a modificar la misma variable
    pthread_create(&hilo1, NULL, incrementar_contador, NULL);
    pthread_create(&hilo2, NULL, incrementar_contador, NULL);

    // Esperar a ambos
    pthread_join(hilo1, NULL);
    pthread_join(hilo2, NULL);

    // Si no usáramos el mutex, el resultado casi nunca sería 200000 debido a la interferencia
    printf("[MAIN] El contador final es: %d (Esperado: 200000)\n", contador);

    // Destruir el mutex al finalizar
    pthread_mutex_destroy(&cerrojo);

    return 0;
}
