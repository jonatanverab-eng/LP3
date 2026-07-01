/*
 * parallel_mergesort.c
 *
 * Implementacion de Merge Sort Paralelo utilizando POSIX Threads (PThreads)
 *
 * Estrategia de paralelizacion:
 *   - El subarreglo izquierdo es procesado por un thread.
 *   - El subarreglo derecho es procesado por otro thread.
 *   - El thread padre espera la finalizacion de ambos mediante pthread_join().
 *   - Posteriormente se realiza la operacion de mezcla (Merge) de ambos
 *     subarreglos ordenados.
 *
 * Cada thread recibe, mediante una estructura, los parametros necesarios
 * para procesar su segmento del arreglo (arreglo, indice inicial e indice
 * final). No se utilizan variables globales para transmitir los limites
 * de cada subarreglo.
 *
 * Para evitar una cantidad excesiva de threads, el algoritmo deja de crear
 * nuevos threads cuando el tamano del subarreglo es menor o igual a la
 * constante MIN_SIZE. En ese caso, el Merge Sort continua ejecutandose de
 * forma secuencial sobre dicho segmento.
 *
 * Compilacion:
 *   gcc -Wall -o parallel_mergesort parallel_mergesort.c -lpthread
 *
 * Ejecucion:
 *   ./parallel_mergesort [N]
 *
 *   Si no se recibe N por linea de comandos, se utiliza un valor por
 *   defecto (DEFAULT_N).
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

/* Tamano del arreglo por defecto, usado si no se recibe N por linea de
 * comandos. */
#define DEFAULT_N 20

/* Umbral a partir del cual el algoritmo deja de crear nuevos threads y
 * continua de forma secuencial. Valor definido libremente por el grupo. */
#define MIN_SIZE 4

/* Estructura utilizada para pasarle a cada thread los parametros
 * necesarios para procesar su propio segmento del arreglo. No se usan
 * variables globales para este proposito. */
typedef struct {
    int *arr;    /* puntero al arreglo (compartido, cada thread solo
                    modifica los elementos de su propio rango) */
    int inicio;  /* indice inicial (inclusive) del segmento */
    int fin;     /* indice final (inclusive) del segmento */
} RangoThread;

/* Prototipos */
void merge_sort_secuencial(int *arr, int inicio, int fin);
void merge(int *arr, int inicio, int medio, int fin);
void *merge_sort_thread(void *arg);
void merge_sort_paralelo(int *arr, int inicio, int fin);
void generar_arreglo(int *arr, int n);
void mostrar_arreglo(const int *arr, int n);

int main(int argc, char *argv[]) {
    int n = DEFAULT_N;

    /* 1. Tamano del arreglo: definido mediante constante o recibido por
     *    linea de comandos. */
    if (argc >= 2) {
        n = atoi(argv[1]);
        if (n <= 0) {
            fprintf(stderr, "N debe ser un entero positivo.\n");
            return EXIT_FAILURE;
        }
    }

    int *arr = malloc((size_t)n * sizeof(int));
    if (arr == NULL) {
        fprintf(stderr, "Error al reservar memoria.\n");
        return EXIT_FAILURE;
    }

    /* 1. Generar arreglo de enteros de tamano N */
    generar_arreglo(arr, n);

    /* 2. Mostrar el arreglo original */
    printf("Arreglo original (N = %d):\n", n);
    mostrar_arreglo(arr, n);

    /* 3. Ejecutar Merge Sort utilizando PThreads */
    merge_sort_paralelo(arr, 0, n - 1);

    /* 4. Mostrar el arreglo ordenado */
    printf("\nArreglo ordenado:\n");
    mostrar_arreglo(arr, n);

    free(arr);
    return EXIT_SUCCESS;
}

/* Genera un arreglo de n enteros con valores aleatorios. */
void generar_arreglo(int *arr, int n) {
    srand((unsigned int)time(NULL));
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 1000; /* valores entre 0 y 999 */
    }
}

/* Muestra el contenido del arreglo por pantalla. */
void mostrar_arreglo(const int *arr, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

/*
 * merge_sort_paralelo
 *
 * Punto de entrada de la version paralela. Decide, en funcion del tamano
 * del segmento [inicio, fin], si debe seguir dividiendo el trabajo
 * creando threads (division recursiva) o si, al haber alcanzado el
 * umbral MIN_SIZE, debe resolver el segmento de forma secuencial.
 */
void merge_sort_paralelo(int *arr, int inicio, int fin) {
    if (inicio >= fin) {
        return; /* 0 o 1 elemento: ya esta ordenado */
    }

    int tamano = fin - inicio + 1;

    /* Si el segmento es menor o igual a MIN_SIZE, se resuelve de forma
     * secuencial para evitar la creacion excesiva de threads. */
    if (tamano <= MIN_SIZE) {
        merge_sort_secuencial(arr, inicio, fin);
        return;
    }

    int medio = inicio + (fin - inicio) / 2;

    /* Se preparan las estructuras con los parametros de cada mitad.
     * Cada thread solo modificara los elementos correspondientes a su
     * propio subarreglo. */
    RangoThread rango_izq = { arr, inicio, medio };
    RangoThread rango_der = { arr, medio + 1, fin };

    pthread_t thread_izq, thread_der;

    /* Creacion de threads: uno procesa el subarreglo izquierdo y otro
     * el subarreglo derecho. */
    pthread_create(&thread_izq, NULL, merge_sort_thread, &rango_izq);
    pthread_create(&thread_der, NULL, merge_sort_thread, &rango_der);

    /* El thread padre espera la finalizacion de ambos threads hijos. */
    pthread_join(thread_izq, NULL);
    pthread_join(thread_der, NULL);

    /* Una vez ordenados ambos subarreglos, se realiza la mezcla. */
    merge(arr, inicio, medio, fin);
}

/*
 * merge_sort_thread
 *
 * Funcion ejecutada por cada thread. Recibe un puntero a una estructura
 * RangoThread con el arreglo y los limites de su segmento, y aplica de
 * forma recursiva el mismo algoritmo de merge sort paralelo sobre dicho
 * segmento.
 */
void *merge_sort_thread(void *arg) {
    RangoThread *rango = (RangoThread *)arg;
    merge_sort_paralelo(rango->arr, rango->inicio, rango->fin);
    return NULL;
}

/*
 * merge_sort_secuencial
 *
 * Version clasica (secuencial) de merge sort, utilizada cuando el
 * tamano del segmento es menor o igual a MIN_SIZE.
 */
void merge_sort_secuencial(int *arr, int inicio, int fin) {
    if (inicio >= fin) {
        return;
    }

    int medio = inicio + (fin - inicio) / 2;

    merge_sort_secuencial(arr, inicio, medio);
    merge_sort_secuencial(arr, medio + 1, fin);
    merge(arr, inicio, medio, fin);
}

/*
 * merge
 *
 * Combina dos subarreglos ordenados arr[inicio..medio] y
 * arr[medio+1..fin] en un unico segmento ordenado arr[inicio..fin].
 * Operacion clasica de mezcla del algoritmo Merge Sort.
 */
void merge(int *arr, int inicio, int medio, int fin) {
    int n_izq = medio - inicio + 1;
    int n_der = fin - medio;

    int *izq = malloc((size_t)n_izq * sizeof(int));
    int *der = malloc((size_t)n_der * sizeof(int));

    if (izq == NULL || der == NULL) {
        fprintf(stderr, "Error al reservar memoria en merge().\n");
        free(izq);
        free(der);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n_izq; i++) {
        izq[i] = arr[inicio + i];
    }
    for (int j = 0; j < n_der; j++) {
        der[j] = arr[medio + 1 + j];
    }

    int i = 0, j = 0, k = inicio;

    while (i < n_izq && j < n_der) {
        if (izq[i] <= der[j]) {
            arr[k++] = izq[i++];
        } else {
            arr[k++] = der[j++];
        }
    }

    while (i < n_izq) {
        arr[k++] = izq[i++];
    }

    while (j < n_der) {
        arr[k++] = der[j++];
    }

    free(izq);
    free(der);
}