#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define N 20          // Tamano del arreglo
#define MIN_SIZE 5    // Tamano minimo para continuar de forma secuencial

// Estructura para pasar parametros a los threads, incluyendo el nivel actual
typedef struct {
    int *arr;
    int left;
    int right;
    int level;        // Nivel de recursion, usado solo para el trazado
} MergeSortArgs;

/*
 * print_segment()
 *
 * Antes: el header y cada numero se imprimian con llamadas a printf()
 * separadas. Cuando dos threads llamaban a print_segment() "al mismo
 * tiempo", el planificador podia intercalar sus salidas en cualquier
 * punto intermedio (por ejemplo, a mitad de la lista de numeros),
 * produciendo lineas mezcladas como se ve en la captura.
 *
 * Ahora: se arma la linea COMPLETA en un buffer con snprintf() y se
 * imprime con UNA SOLA llamada (fputs). Una unica llamada a una
 * funcion de stdio es atomica respecto de otros threads escribiendo
 * sobre el mismo stream (glibc bloquea internamente el FILE* durante
 * la duracion de la llamada), por lo que la linea sale siempre entera
 * y nunca se mezcla con la de otro thread.
 *
 * Nota: esto garantiza que CADA LINEA se imprime sin corromperse.
 * El ORDEN relativo entre lineas de threads distintos que corren en
 * paralelo puede variar de una ejecucion a otra (eso es normal en
 * concurrencia real), pero cada linea individual siempre se lee bien.
 */
void print_segment(int arr[], int left, int right, int level) {
    char buffer[2048];
    int pos = 0;

    // Indentacion proporcional al nivel, para ver la jerarquia
    for (int i = 0; i < level && pos < (int)sizeof(buffer) - 1; i++) {
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "    ");
    }

    pos += snprintf(buffer + pos, sizeof(buffer) - pos,
                     "[Nivel %d] (izq: %2d, der: %2d) -> ", level, left, right);

    for (int i = left; i <= right && pos < (int)sizeof(buffer) - 8; i++) {
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%d ", arr[i]);
    }

    snprintf(buffer + pos, sizeof(buffer) - pos, "\n");

    // Una unica llamada de impresion => la linea sale atomica y completa
    fputs(buffer, stdout);
}

// Funcion para mezclar dos subarreglos ordenados
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

// Merge Sort Secuencial (sin impresion: los pasos internos del tramo
// que ya paso a modo secuencial no se trazan, solo su segmento inicial,
// que ya se imprimio antes de llamar a esta funcion)
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

        // Los argumentos se reservan en el HEAP (no en la pila de esta
        // funcion) para que cada thread tenga su propia copia estable,
        // sin depender de que el padre no retorne antes de tiempo.
        MergeSortArgs *left_args = malloc(sizeof(MergeSortArgs));
        MergeSortArgs *right_args = malloc(sizeof(MergeSortArgs));
        *left_args  = (MergeSortArgs){arr, left, mid, level + 1};
        *right_args = (MergeSortArgs){arr, mid + 1, right, level + 1};

        pthread_t left_thread, right_thread;

        // Creacion de threads hijos
        pthread_create(&left_thread, NULL, parallel_mergesort, left_args);
        pthread_create(&right_thread, NULL, parallel_mergesort, right_args);

        // Esperar la sincronizacion de los hilos de este nivel
        pthread_join(left_thread, NULL);
        pthread_join(right_thread, NULL);

        free(left_args);
        free(right_args);

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

    printf("=== Traza de Ejecucion por Niveles ===\n");
    // Iniciamos en el nivel 0
    MergeSortArgs main_args = {arr, 0, N - 1, 0};
    parallel_mergesort(&main_args);
    printf("\n");

    printf("=== Arreglo Ordenado Final ===\n");
    for (int i = 0; i < N; i++) printf("%d ", arr[i]);
    printf("\n");

    return 0;
}