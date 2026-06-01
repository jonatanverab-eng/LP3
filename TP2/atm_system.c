/*
 * atm_system.c
 * Simulación de Cajeros Automáticos
 *
 * Objetivo: Aplicar sincronización con mutex y threads POSIX simulando
 * múltiples operaciones concurrentes sobre cuentas bancarias.
 *
 * Compilar con: gcc -o atm_system atm_system.c -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* ─────────────────────────────────────────────────────────── */
/*  Configuración                                              */
/* ─────────────────────────────────────────────────────────── */
#define NUM_CUENTAS    5      /* número de cuentas bancarias  */
#define NUM_CAJEROS    4      /* número de cajeros (threads)  */
#define OPS_POR_CAJERO 8      /* operaciones que hace c/cajero */
#define SALDO_INICIAL  10000  /* saldo inicial de cada cuenta */
#define MAX_MONTO      3000   /* monto máximo por operación   */

/* ─────────────────────────────────────────────────────────── */
/*  Tipos de operación                                         */
/* ─────────────────────────────────────────────────────────── */
typedef enum {
    OP_DEPOSITO = 0,
    OP_EXTRACCION,
    OP_CONSULTA,
    OP_TOTAL          /* centinela, no es una operación real  */
} TipoOperacion;

const char *nombre_op[] = { "DEPÓSITO   ", "EXTRACCIÓN ", "CONSULTA   " };

/* ─────────────────────────────────────────────────────────── */
/*  Estructura de cuenta bancaria                              */
/* ─────────────────────────────────────────────────────────── */
typedef struct {
    int            numero;   /* número de cuenta              */
    double         saldo;    /* saldo actual                  */
    pthread_mutex_t mutex;   /* mutex propio de la cuenta     */
} Cuenta;

/* ─────────────────────────────────────────────────────────── */
/*  Estructura de argumentos para cada cajero                  */
/* ─────────────────────────────────────────────────────────── */
typedef struct {
    int     id;              /* identificador del cajero      */
    Cuenta *cuentas;         /* puntero al arreglo de cuentas */
    int     num_cuentas;
} ArgsCajero;

/* ─────────────────────────────────────────────────────────── */
/*  Mutex global para la salida por consola (printf/log)       */
/* ─────────────────────────────────────────────────────────── */
static pthread_mutex_t mutex_consola = PTHREAD_MUTEX_INITIALIZER;

/* ─────────────────────────────────────────────────────────── */
/*  Funciones de negocio (siempre invocadas con mutex tomado)  */
/* ─────────────────────────────────────────────────────────── */

/* Depósito: siempre válido */
static void depositar(Cuenta *c, double monto, int cajero_id)
{
    c->saldo += monto;

    pthread_mutex_lock(&mutex_consola);
    printf("[Cajero %d] %s | Cuenta #%02d | Monto: +%8.2f | Saldo resultante: %10.2f\n",
           cajero_id, nombre_op[OP_DEPOSITO], c->numero, monto, c->saldo);
    pthread_mutex_unlock(&mutex_consola);
}

/* Extracción: sólo si hay saldo suficiente */
static void extraer(Cuenta *c, double monto, int cajero_id)
{
    if (c->saldo < monto) {
        pthread_mutex_lock(&mutex_consola);
        printf("[Cajero %d] %s | Cuenta #%02d | Monto: -%8.2f | RECHAZADA - Saldo insuficiente (%.2f)\n",
               cajero_id, nombre_op[OP_EXTRACCION], c->numero, monto, c->saldo);
        pthread_mutex_unlock(&mutex_consola);
        return;
    }

    c->saldo -= monto;

    pthread_mutex_lock(&mutex_consola);
    printf("[Cajero %d] %s | Cuenta #%02d | Monto: -%8.2f | Saldo resultante: %10.2f\n",
           cajero_id, nombre_op[OP_EXTRACCION], c->numero, monto, c->saldo);
    pthread_mutex_unlock(&mutex_consola);
}

/* Consulta de saldo: sólo lectura */
static void consultar(Cuenta *c, int cajero_id)
{
    pthread_mutex_lock(&mutex_consola);
    printf("[Cajero %d] %s | Cuenta #%02d | Saldo actual:  %10.2f\n",
           cajero_id, nombre_op[OP_CONSULTA], c->numero, c->saldo);
    pthread_mutex_unlock(&mutex_consola);
}

/* ─────────────────────────────────────────────────────────── */
/*  Función del thread cajero                                  */
/* ─────────────────────────────────────────────────────────── */
static void *rutina_cajero(void *arg)
{
    ArgsCajero *args = (ArgsCajero *)arg;
    int id           = args->id;
    Cuenta *cuentas  = args->cuentas;
    int n            = args->num_cuentas;

    /* Semilla distinta por thread */
    unsigned int seed = (unsigned int)(time(NULL) ^ (id * 0x9e3779b9u));

    for (int op_num = 0; op_num < OPS_POR_CAJERO; op_num++) {
        /* Seleccionar cuenta y operación aleatoriamente */
        int idx_cuenta  = rand_r(&seed) % n;
        TipoOperacion t = (TipoOperacion)(rand_r(&seed) % OP_TOTAL);
        double monto    = (double)(rand_r(&seed) % MAX_MONTO + 100);

        Cuenta *c = &cuentas[idx_cuenta];

        /* ── Sección crítica: protege la cuenta individual ── */
        pthread_mutex_lock(&c->mutex);
        switch (t) {
            case OP_DEPOSITO:   depositar(c, monto, id); break;
            case OP_EXTRACCION: extraer(c, monto, id);   break;
            case OP_CONSULTA:   consultar(c, id);         break;
            default: break;
        }
        pthread_mutex_unlock(&c->mutex);
        /* ────────────────────────────────────────────────── */

        /* Pequeña pausa para intercalar operaciones */
        usleep((rand_r(&seed) % 100) * 1000);
    }

    pthread_mutex_lock(&mutex_consola);
    printf("[Cajero %d] Finalizó todas sus operaciones.\n", id);
    pthread_mutex_unlock(&mutex_consola);

    return NULL;
}

/* ─────────────────────────────────────────────────────────── */
/*  Función auxiliar: imprimir estado final de cuentas         */
/* ─────────────────────────────────────────────────────────── */
static void imprimir_resumen(Cuenta *cuentas, int n)
{
    double total = 0.0;

    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║       ESTADO FINAL DE LAS CUENTAS        ║\n");
    printf("╠══════════════════════════════════════════╣\n");
    for (int i = 0; i < n; i++) {
        printf("║  Cuenta #%02d  →  Saldo: %14.2f    ║\n",
               cuentas[i].numero, cuentas[i].saldo);
        total += cuentas[i].saldo;
    }
    printf("╠══════════════════════════════════════════╣\n");
    printf("║  TOTAL SISTEMA  →  %18.2f    ║\n", total);
    printf("╚══════════════════════════════════════════╝\n");
}

/* ─────────────────────────────────────────────────────────── */
/*  main                                                       */
/* ─────────────────────────────────────────────────────────── */
int main(void)
{
    srand((unsigned int)time(NULL));

    /* ── Inicializar cuentas ── */
    Cuenta cuentas[NUM_CUENTAS];
    double total_inicial = 0.0;

    printf("══════════════════════════════════════════════\n");
    printf("   SIMULACIÓN DE CAJEROS AUTOMÁTICOS (POSIX)  \n");
    printf("══════════════════════════════════════════════\n\n");

    printf("Cuentas inicializadas:\n");
    for (int i = 0; i < NUM_CUENTAS; i++) {
        cuentas[i].numero = 1000 + i;
        cuentas[i].saldo  = SALDO_INICIAL;
        pthread_mutex_init(&cuentas[i].mutex, NULL);
        total_inicial += cuentas[i].saldo;
        printf("  Cuenta #%02d  →  Saldo inicial: %.2f\n",
               cuentas[i].numero, cuentas[i].saldo);
    }
    printf("  Total inicial del sistema: %.2f\n\n", total_inicial);

    /* ── Crear threads cajeros ── */
    pthread_t      threads[NUM_CAJEROS];
    ArgsCajero     args[NUM_CAJEROS];

    printf("Iniciando %d cajeros con %d operaciones cada uno…\n\n",
           NUM_CAJEROS, OPS_POR_CAJERO);

    for (int i = 0; i < NUM_CAJEROS; i++) {
        args[i].id          = i + 1;
        args[i].cuentas     = cuentas;
        args[i].num_cuentas = NUM_CUENTAS;

        if (pthread_create(&threads[i], NULL, rutina_cajero, &args[i]) != 0) {
            perror("Error al crear thread");
            exit(EXIT_FAILURE);
        }
    }

    /* ── Esperar a que terminen todos los cajeros ── */
    for (int i = 0; i < NUM_CAJEROS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* ── Resumen final ── */
    imprimir_resumen(cuentas, NUM_CUENTAS);

    /* ── Limpiar recursos ── */
    for (int i = 0; i < NUM_CUENTAS; i++) {
        pthread_mutex_destroy(&cuentas[i].mutex);
    }
    pthread_mutex_destroy(&mutex_consola);

    return 0;
}
