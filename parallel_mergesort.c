#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define N 20          // Tamaño del arreglo
#define MIN_SIZE 5    // Tamaño mínimo para continuar de forma secuencial

// Estructura para pasar parámetros a los threads, incluyendo el nivel actual
typedef struct {
    int *arr;
    int left;
    int right;
    int level;        // Añadido para rastrear el nivel de recursión
} MergeSortArgs;

// Función para imprimir un segmento específico del arreglo con tabulación por nivel
void print_segment(int arr[], int left, int right, int level) {
    // Tabulación proporcional al nivel para que se note la jerarquía visualmente
    for (int i = 0; i < level; i++) printf("    ");
    
    printf("[Nivel %d] (izq: %d, der: %d): ", level, left, right);
    for (int i = left; i <= right; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

// Función para mezclar dos subarreglos ordenados
void merge(int arr[], int left, int mid, int right) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

    for (i = 0; i < n1; i++) L[i] = arr[left + i];
    for (j = 0; j < n2; j++) R[j] = arr[mid + 1 + j];

    i = 0; j = 0; k = left;

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
        i++; k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++; k++;
    }

    free(L);
    free(R);
}

// Merge Sort Secuencial (mantiene el rastreo de nivel para las impresiones finales)
void sequential_mergesort(int arr[], int left, int right, int level) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        sequential_mergesort(arr, left, mid, level + 1);
        sequential_mergesort(arr, mid + 1, right, level + 1);
        merge(arr, left, mid, right);
    }
}

// Merge Sort Paralelo
void *parallel_mergesort(void *arguments) {
    MergeSortArgs *args = (MergeSortArgs *)arguments;
    int left = args->left;
    int right = args->right;
    int *arr = args->arr;
    int level = args->level;

    if (left < right) {
        int size = right - left + 1;

        // Imprime el subarreglo asignado a este nivel antes de procesarlo
        print_segment(arr, left, right, level);

        // Si es menor o igual a MIN_SIZE, pasa a secuencial de forma interna
        if (size <= MIN_SIZE) {
            sequential_mergesort(arr, left, right, level);
            return NULL;
        }

        int mid = left + (right - left) / 2;

        // Configurar argumentos incrementando el nivel (+ 1)
        MergeSortArgs left_args = {arr, left, mid, level + 1};
        pthread_t left_thread;

        MergeSortArgs right_args = {arr, mid + 1, right, level + 1};
        pthread_t right_thread;

        // Creación de threads hijos
        pthread_create(&left_thread, NULL, parallel_mergesort, &left_args);
        pthread_create(&right_thread, NULL, parallel_mergesort, &right_args);

        // Esperar la sincronización de los hilos de este nivel
        pthread_join(left_thread, NULL);
        pthread_join(right_thread, NULL);

        // Mezclar los resultados
        merge(arr, left, mid, right);
    }
    return NULL;
}

int main() {
    int arr[N];
    srand(time(NULL));

    for (int i = 0; i < N; i++) {
        arr[i] = rand() % 100;
    }

    printf("=== Arreglo Inicial ===\n");
    for (int i = 0; i < N; i++) printf("%d ", arr[i]);
    printf("\n\n");

    printf("=== Traza de Ejecución por Niveles ===\n");
    // Iniciamos en el nivel 0
    MergeSortArgs main_args = {arr, 0, N - 1, 0};
    parallel_mergesort(&main_args);
    printf("\n");

    printf("=== Arreglo Ordenado Final ===\n");
    for (int i = 0; i < N; i++) printf("%d ", arr[i]);
    printf("\n");

    return 0;
}