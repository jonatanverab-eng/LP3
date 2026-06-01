/*
 * atm_cajeros_v2.c
 * Simulación de Cajeros Automáticos
 *
 * - La cantidad de cajeros (hilos) se recibe por línea de comandos.
 * - Las cuentas son globales y fijas (definidas en duro).
 * - Los hilos cajeros no reciben struct de atributos; sólo su ID (int).
 *
 * Compilar: gcc -o atm_cajeros_v2 atm_cajeros_v2.c -lpthread
 * Uso:      ./atm_cajeros_v2 <num_cajeros>
 * Ejemplo:  ./atm_cajeros_v2 5
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* ── Configuración fija ──────────────────────────────────── */
#define NUM_CUENTAS          5
#define OPERACIONES_POR_CAJERO 4
#define MIN_CAJEROS          1
#define MAX_CAJEROS          20

/* ── Estructura de cuenta bancaria (global) ─────────────── */
typedef struct {
    int numero_cuenta;
    double saldo;
    pthread_mutex_t mutex;
} CuentaBancaria;

/* Las cuentas están definidas en duro como variable global */
static CuentaBancaria cuentas[NUM_CUENTAS];

/* ── Función del hilo cajero ─────────────────────────────── */
/* Recibe únicamente su ID, no una estructura de atributos   */
void* simular_cajero(void* arg) {
    int id = *((int*)arg);
    free(arg); /* liberar el entero asignado en main */

    unsigned int semilla = (unsigned int)(time(NULL) ^ ((unsigned int)id << 16));

    for (int i = 0; i < OPERACIONES_POR_CAJERO; i++) {
        int    idx_cuenta = rand_r(&semilla) % NUM_CUENTAS;
        int    tipo_op    = rand_r(&semilla) % 3; /* 0=Depósito 1=Extracción 2=Consulta */
        double monto      = 10.0 + (rand_r(&semilla) % 191);

        CuentaBancaria *cuenta = &cuentas[idx_cuenta];

        pthread_mutex_lock(&cuenta->mutex);

        printf("[Cajero %d] Operación en Cuenta N° %d...\n", id, cuenta->numero_cuenta);

        if (tipo_op == 0) {
            cuenta->saldo += monto;
            printf("  -> DEPÓSITO de $%.2f | Saldo: $%.2f\n", monto, cuenta->saldo);
        } else if (tipo_op == 1) {
            printf("  -> EXTRACCIÓN de $%.2f\n", monto);
            if (cuenta->saldo >= monto) {
                cuenta->saldo -= monto;
                printf("  -> RESULTADO: Éxito. Saldo: $%.2f\n", cuenta->saldo);
            } else {
                printf("  -> RESULTADO: RECHAZADO. Saldo insuficiente ($%.2f).\n", cuenta->saldo);
            }
        } else {
            printf("  -> CONSULTA | Saldo actual: $%.2f\n", cuenta->saldo);
        }

        printf("----------------------------------------\n");
        pthread_mutex_unlock(&cuenta->mutex);

        usleep((rand_r(&semilla) % 500 + 100) * 1000);
    }

    pthread_exit(NULL);
}

/* ── main ────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <num_cajeros>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_cajeros = atoi(argv[1]);
    if (num_cajeros < MIN_CAJEROS || num_cajeros > MAX_CAJEROS) {
        fprintf(stderr, "Error: num_cajeros debe estar entre %d y %d.\n",
                MIN_CAJEROS, MAX_CAJEROS);
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));

    /* Inicializar cuentas globales (en duro) */
    int numeros_fijos[NUM_CUENTAS]  = {1001, 1002, 1003, 1004, 1005};
    double saldos_fijos[NUM_CUENTAS] = {500.0, 1200.0, 300.0, 850.0, 2000.0};

    for (int i = 0; i < NUM_CUENTAS; i++) {
        cuentas[i].numero_cuenta = numeros_fijos[i];
        cuentas[i].saldo         = saldos_fijos[i];
        if (pthread_mutex_init(&cuentas[i].mutex, NULL) != 0) {
            perror("Error al inicializar mutex");
            return EXIT_FAILURE;
        }
    }

    printf("=== SIMULACIÓN DE CAJEROS AUTOMÁTICOS ===\n");
    printf("Cuentas (fijas): %d | Cajeros (hilos): %d\n\n", NUM_CUENTAS, num_cajeros);

    /* Crear hilos; cada uno solo recibe su ID */
    pthread_t *hilos = malloc(num_cajeros * sizeof(pthread_t));
    if (!hilos) { perror("malloc"); return EXIT_FAILURE; }

    for (int i = 0; i < num_cajeros; i++) {
        int *id = malloc(sizeof(int));
        if (!id) { perror("malloc id"); return EXIT_FAILURE; }
        *id = i + 1;

        if (pthread_create(&hilos[i], NULL, simular_cajero, id) != 0) {
            perror("Error al crear hilo");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < num_cajeros; i++)
        pthread_join(hilos[i], NULL);

    printf("\n=== ESTADO FINAL DE CUENTAS ===\n");
    for (int i = 0; i < NUM_CUENTAS; i++) {
        printf("Cuenta N° %d - Saldo Final: $%.2f\n",
               cuentas[i].numero_cuenta, cuentas[i].saldo);
        pthread_mutex_destroy(&cuentas[i].mutex);
    }

    free(hilos);
    return 0;
}
