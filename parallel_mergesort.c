#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define N 20          // Tamaño del arreglo
#define MIN_SIZE 5    // Tamaño mínimo para continuar de forma secuencial

// Estructura para pasar parámetros a los threads
typedef struct {
    int *arr;
    int left;
    int right;
} MergeSortArgs;

// Función para mezclar dos subarreglos ordenados
void merge(int arr[], int left, int mid, int right) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

    for (i = 0; i < n1; i++) L[i] = arr[left + i];
    for (j = 0; j < n2; j++) R[j] = arr[mid + 1 + j];

    i = 0;
    j = 0;
    k = left;

    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

    free(L);
    free(R);
}

// Merge Sort Secuencial
void sequential_mergesort(int arr[], int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        sequential_mergesort(arr, left, mid);
        sequential_mergesort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

// Merge Sort Paralelo (Función que ejecutan los threads)
void *parallel_mergesort(void *arguments) {
    MergeSortArgs *args = (MergeSortArgs *)arguments;
    int left = args->left;
    int right = args->right;
    int *arr = args->arr;

    if (left < right) {
        int size = right - left + 1;

        // Si el tamaño del subarreglo es menor o igual a MIN_SIZE, se ejecuta de forma secuencial
        if (size <= MIN_SIZE) {
            sequential_mergesort(arr, left, right);
            return NULL;
        }

        int mid = left + (right - left) / 2;

        // Configurar argumentos para el subarreglo izquierdo
        MergeSortArgs left_args = {arr, left, mid};
        pthread_t left_thread;

        // Configurar argumentos para el subarreglo derecho
        MergeSortArgs right_args = {arr, mid + 1, right};
        pthread_t right_thread;

        // Crear los threads para procesar ambos subarreglos concurrentemente
        pthread_create(&left_thread, NULL, parallel_mergesort, &left_args);
        pthread_create(&right_thread, NULL, parallel_mergesort, &right_args);

        // Esperar a que ambos threads terminen (Sincronización)
        pthread_join(left_thread, NULL);
        pthread_join(right_thread, NULL);

        // Realizar la mezcla de los subarreglos ordenados por el thread padre
        merge(arr, left, mid, right);
    }
    return NULL;
}

int main() {
    int arr[N];
    srand(time(NULL));

    // 1. Generar arreglo de enteros aleatorios
    for (int i = 0; i < N; i++) {
        arr[i] = rand() % 100;
    }

    // 2. Mostrar el arreglo original
    printf("Arreglo original:\n");
    for (int i = 0; i < N; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n\n");

    // 3. Ejecutar Merge Sort utilizando PThreads
    MergeSortArgs main_args = {arr, 0, N - 1};
    parallel_mergesort(&main_args);

    // 4. Mostrar el arreglo ordenado
    printf("Arreglo ordenado:\n");
    for (int i = 0; i < N; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    return 0;
}