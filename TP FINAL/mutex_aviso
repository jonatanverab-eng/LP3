#include <stdio.h>
#include <pthread.h>
#include <errno.h> // Necesario para detectar el error EBUSY

int contador = 0; 
pthread_mutex_t cerrojo = PTHREAD_MUTEX_INITIALIZER; 

void* incrementar_contador(void* arg) {
    long id = (long)arg;

    for (int i = 0; i < 5; i++) {
        // pthread_mutex_trylock intenta tomar el mutex. 
        // Si devuelve 0, tuvo éxito. Si devuelve EBUSY, significa que está ocupado.
        while (pthread_mutex_trylock(&cerrojo) == EBUSY) {
            // Entra aquí SOLO si el mutex está siendo usado por el otro hilo
            printf("[HILO %ld] El mutex está OCUPADO. Estoy esperando...\n", id);
            
            // Una pequeña pausa para no saturar la pantalla con texto
            struct timespec ts = {0, 100000000}; // 100 milisegundos
            nanosleep(&ts, NULL);
        }
        
        // --- INICIO DE LA ZONA CRÍTICA (Ya tenemos el cerrojo) ---
        printf("[HILO %ld] ---> Entré a la zona crítica (Tengo el mutex)\n", id);
        contador++; 
        
        // Simulamos que el hilo se tarda un poco trabajando dentro de la zona crítica
        struct timespec ts_trabajo = {0, 200000000}; // 200 milisegundos
        nanosleep(&ts_trabajo, NULL);
        // --- FIN DE LA ZONA CRÍTICA ---
        
        // Liberamos el mutex para el siguiente
        pthread_mutex_unlock(&cerrojo);
        
        // Pequeño descanso fuera del mutex para dar oportunidad al otro hilo de entrar
        struct timespec ts_descanso = {0, 50000000}; 
        nanosleep(&ts_descanso, NULL);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t hilo1, hilo2;

    // Pasamos el número 1 y 2 como identificadores de los hilos
    pthread_create(&hilo1, NULL, incrementar_contador, (void*)1);
    pthread_create(&hilo2, NULL, incrementar_contador, (void*)2);

    pthread_join(hilo1, NULL);
    pthread_join(hilo2, NULL);

    printf("[MAIN] El contador final es: %d\n", contador);
    pthread_mutex_destroy(&cerrojo);

    return 0;
}
