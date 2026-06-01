/*
 * atm_system_v2.c
 * Simulación de Cajeros Automáticos
 *
 * - La cantidad de cajeros (hilos) se recibe por línea de comandos.
 * - Las cuentas son globales y fijas (definidas en duro).
 * - Los hilos cajeros no reciben struct de atributos; sólo su ID (int).
 *
 * Compilar: gcc -o atm_system_v2 atm_system_v2.c -lpthread
 * Uso:      ./atm_system_v2 <num_cajeros>
 * Ejemplo:  ./atm_system_v2 6
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* ── Configuración fija ──────────────────────────────────── */
#define NUM_CUENTAS    5
#define OPS_POR_CAJERO 8
#define MAX_MONTO      3000
#define MIN_CAJEROS    1
#define MAX_CAJEROS    50

/* ── Tipos de operación ──────────────────────────────────── */
typedef enum {
    OP_DEPOSITO = 0,
    OP_EXTRACCION,
    OP_CONSULTA,
    OP_TOTAL
} TipoOperacion;

static const char *nombre_op[] = { "DEPÓSITO   ", "EXTRACCIÓN ", "CONSULTA   " };

/* ── Estructura de cuenta bancaria ──────────────────────── */
typedef struct {
    int             numero;
    double          saldo;
    pthread_mutex_t mutex;
} Cuenta;

/* Las cuentas están definidas en duro como variable global  */
static Cuenta cuentas[NUM_CUENTAS];

/* Mutex global para salida por consola */
static pthread_mutex_t mutex_consola = PTHREAD_MUTEX_INITIALIZER;

/* ── Operaciones de negocio ──────────────────────────────── */
static void depositar(Cuenta *c, double monto, int cajero_id)
{
    c->saldo += monto;
    pthread_mutex_lock(&mutex_consola);
    printf("[Cajero %d] %s | Cuenta #%04d | +%8.2f | Saldo: %10.2f\n",
           cajero_id, nombre_op[OP_DEPOSITO], c->numero, monto, c->saldo);
    pthread_mutex_unlock(&mutex_consola);
}

static void extraer(Cuenta *c, double monto, int cajero_id)
{
    if (c->saldo < monto) {
        pthread_mutex_lock(&mutex_consola);
        printf("[Cajero %d] %s | Cuenta #%04d | -%8.2f | RECHAZADA (saldo: %.2f)\n",
               cajero_id, nombre_op[OP_EXTRACCION], c->numero, monto, c->saldo);
        pthread_mutex_unlock(&mutex_consola);
        return;
    }
    c->saldo -= monto;
    pthread_mutex_lock(&mutex_consola);
    printf("[Cajero %d] %s | Cuenta #%04d | -%8.2f | Saldo: %10.2f\n",
           cajero_id, nombre_op[OP_EXTRACCION], c->numero, monto, c->saldo);
    pthread_mutex_unlock(&mutex_consola);
}

static void consultar(Cuenta *c, int cajero_id)
{
    pthread_mutex_lock(&mutex_consola);
    printf("[Cajero %d] %s | Cuenta #%04d | Saldo: %10.2f\n",
           cajero_id, nombre_op[OP_CONSULTA], c->numero, c->saldo);
    pthread_mutex_unlock(&mutex_consola);
}

/* ── Función del hilo cajero ─────────────────────────────── */
/* Recibe únicamente su ID, no una estructura de atributos   */
static void *rutina_cajero(void *arg)
{
    int id = *((int*)arg);
    free(arg); /* liberar el entero asignado en main */

    unsigned int seed = (unsigned int)(time(NULL) ^ ((unsigned int)id * 0x9e3779b9u));

    for (int op = 0; op < OPS_POR_CAJERO; op++) {
        int idx           = rand_r(&seed) % NUM_CUENTAS;
        TipoOperacion t   = (TipoOperacion)(rand_r(&seed) % OP_TOTAL);
        double monto      = (double)(rand_r(&seed) % MAX_MONTO + 100);

        Cuenta *c = &cuentas[idx];

        pthread_mutex_lock(&c->mutex);
        switch (t) {
            case OP_DEPOSITO:   depositar(c, monto, id); break;
            case OP_EXTRACCION: extraer(c, monto, id);   break;
            case OP_CONSULTA:   consultar(c, id);         break;
            default: break;
        }
        pthread_mutex_unlock(&c->mutex);

        usleep((rand_r(&seed) % 100) * 1000);
    }

    pthread_mutex_lock(&mutex_consola);
    printf("[Cajero %d] Finalizó todas sus operaciones.\n", id);
    pthread_mutex_unlock(&mutex_consola);

    return NULL;
}

/* ── Resumen final ───────────────────────────────────────── */
static void imprimir_resumen(void)
{
    double total = 0.0;
    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║       ESTADO FINAL DE LAS CUENTAS        ║\n");
    printf("╠══════════════════════════════════════════╣\n");
    for (int i = 0; i < NUM_CUENTAS; i++) {
        printf("║  Cuenta #%04d  →  Saldo: %12.2f    ║\n",
               cuentas[i].numero, cuentas[i].saldo);
        total += cuentas[i].saldo;
    }
    printf("╠══════════════════════════════════════════╣\n");
    printf("║  TOTAL SISTEMA  →  %18.2f    ║\n", total);
    printf("╚══════════════════════════════════════════╝\n");
}

/* ── main ────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <num_cajeros>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *endptr;
    long n = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || n < MIN_CAJEROS || n > MAX_CAJEROS) {
        fprintf(stderr, "Error: num_cajeros debe ser un entero entre %d y %d.\n",
                MIN_CAJEROS, MAX_CAJEROS);
        return EXIT_FAILURE;
    }
    int num_cajeros = (int)n;

    srand((unsigned int)time(NULL));

    /* Inicializar cuentas globales (en duro) */
    int    numeros[NUM_CUENTAS] = {1001, 1002, 1003, 1004, 1005};
    double saldos[NUM_CUENTAS]  = {10000.0, 8500.0, 12000.0, 5000.0, 15000.0};

    printf("══════════════════════════════════════════════\n");
    printf("   SIMULACIÓN DE CAJEROS AUTOMÁTICOS (POSIX)  \n");
    printf("══════════════════════════════════════════════\n\n");
    printf("Cuentas (fijas: %d):\n", NUM_CUENTAS);

    double total_inicial = 0.0;
    for (int i = 0; i < NUM_CUENTAS; i++) {
        cuentas[i].numero = numeros[i];
        cuentas[i].saldo  = saldos[i];
        pthread_mutex_init(&cuentas[i].mutex, NULL);
        total_inicial += cuentas[i].saldo;
        printf("  Cuenta #%04d  →  Saldo inicial: %.2f\n", cuentas[i].numero, cuentas[i].saldo);
    }
    printf("  Total inicial: %.2f\n\n", total_inicial);
    printf("Iniciando %d cajero(s) con %d operaciones cada uno…\n\n",
           num_cajeros, OPS_POR_CAJERO);

    /* Crear hilos; cada uno solo recibe su ID */
    pthread_t *threads = malloc(num_cajeros * sizeof(pthread_t));
    if (!threads) { perror("malloc"); return EXIT_FAILURE; }

    for (int i = 0; i < num_cajeros; i++) {
        int *id = malloc(sizeof(int));
        if (!id) { perror("malloc id"); return EXIT_FAILURE; }
        *id = i + 1;

        if (pthread_create(&threads[i], NULL, rutina_cajero, id) != 0) {
            perror("Error al crear thread");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < num_cajeros; i++)
        pthread_join(threads[i], NULL);

    imprimir_resumen();

    for (int i = 0; i < NUM_CUENTAS; i++)
        pthread_mutex_destroy(&cuentas[i].mutex);
    pthread_mutex_destroy(&mutex_consola);

    free(threads);
    return 0;
}
