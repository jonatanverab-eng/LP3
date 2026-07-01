#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// Estructura para pasar parámetros a cada hilo
typedef struct {
    int *array;
    int left;
    int right;
    int depth;
} ThreadParams;

// MAX_DEPTH controla cuántos niveles de hilos se crearán en total
// El total de hilos creados será (2^(MAX_DEPTH + 1) - 2) en el árbol de recursión
#define MAX_DEPTH 2 

// Función tradicional para mezclar (merge) dos mitades de un array
void merge(int *array, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

    for (int i = 0; i < n1; i++) L[i] = array[left + i];
    for (int j = 0; j < n2; j++) R[j] = array[mid + 1 + j];

    int i = 0, j = 0, k = left;

    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            array[k] = L[i++];
        } else {
            array[k] = R[j++];
        }
        k++;
    }

    while (i < n1) array[k++] = L[i++];
    while (j < n2) array[k++] = R[j++];

    free(L);
    free(R);
}

// Función tradicional de Merge Sort secuencial
void mergeSort(int *array, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(array, left, mid);
        mergeSort(array, mid + 1, right);
        merge(array, left, mid, right);
    }
}

// Función que ejecutan los hilos
void *threadedMergeSort(void *arg) {
    ThreadParams *params = (ThreadParams *)arg;
    int left = params->left;
    int right = params->right;
    int depth = params->depth;
    int *array = params->array;

    if (left < right) {
        int mid = left + (right - left) / 2;

        if (depth > 0) {
            // Todavía podemos crear más hilos (no hemos llegado a la profundidad máxima)
            pthread_t leftThread, rightThread;
            ThreadParams leftParams = {array, left, mid, depth - 1};
            ThreadParams rightParams = {array, mid + 1, right, depth - 1};

            // Se crean dos nuevos hilos para ordenar las mitades izquierda y derecha
            pthread_create(&leftThread, NULL, threadedMergeSort, &leftParams);
            pthread_create(&rightThread, NULL, threadedMergeSort, &rightParams);

            // Se espera a que ambos hilos terminen su trabajo
            pthread_join(leftThread, NULL);
            pthread_join(rightThread, NULL);
        } else {
            // Si ya se alcanzó la profundidad máxima de hilos, se ordena de manera secuencial (sin más hilos)
            mergeSort(array, left, mid);
            mergeSort(array, mid + 1, right);
        }

        // Finalmente se mezclan las dos mitades ordenadas
        merge(array, left, mid, right);
    }

    return NULL;
}

int main() {
    int size = 20; // Tamaño del array de prueba
    int *array = (int *)malloc(size * sizeof(int));

    // Inicializar el array con números aleatorios
    srand((unsigned int)time(NULL));
    printf("Array original:\n");
    for (int i = 0; i < size; i++) {
        array[i] = rand() % 100;
        printf("%d ", array[i]);
    }
    printf("\n\n");

    // Parámetros iniciales: pasamos todo el array y la profundidad máxima permitida
    ThreadParams initialParams = {array, 0, size - 1, MAX_DEPTH};

    // Comenzar el ordenamiento (esto creará hilos recursivamente)
    threadedMergeSort(&initialParams);

    printf("Array ordenado:\n");
    for (int i = 0; i < size; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");

    free(array);
    return 0;
}
