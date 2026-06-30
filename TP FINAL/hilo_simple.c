#include <stdio.h>
#include <pthread.h>

// Esta es la función que ejecutará el hilo secundario
void* saludar(void* arg) {
    printf("¡Hola Mundo desde el HILO SECUNDARIO!\n");
    return NULL;
}

int main() {
    pthread_t hilo; // El identificador de nuestro hilo

    // 1. Creamos el hilo y le asignamos la función 'saludar'
    pthread_create(&hilo, NULL, saludar, NULL);

    // 2. El hilo principal (main) saluda por su cuenta
    printf("¡Hola Mundo desde el HILO PRINCIPAL (Main)!\n");

    // 3. Esperamos a que el hilo secundario termine antes de cerrar el programa
    pthread_join(hilo, NULL);

    return 0;
}
