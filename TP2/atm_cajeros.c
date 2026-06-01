#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_CUENTAS 5
#define NUM_CAJEROS 3
#define OPERACIONES_POR_CAJERO 4

// Estructura que representa una cuenta bancaria
typedef struct {
    int numero_cuenta;
    double saldo;
    pthread_mutex_t mutex; // Mutex por cuenta para proteger su saldo de accesos concurrentes
} CuentaBancaria;

// Estructura para pasar datos a los hilos (cajeros)
typedef struct {
    int id_cajero;
    CuentaBancaria *cuentas;
} DatosCajero;

// Función que ejecutarán los hilos (cajeros automáticos)
void* simular_cajero(void* arg) {
    DatosCajero *datos = (DatosCajero*) arg;
    int id = datos->id_cajero;
    CuentaBancaria *cuentas = datos->cuentas;

    // Inicializar la semilla aleatoria de forma independiente para cada hilo
    unsigned int semilla = time(NULL) ^ (id << 16);

    for (int i = 0; i < OPERACIONES_POR_CAJERO; i++) {
        // Seleccionar una cuenta aleatoria (0 a NUM_CUENTAS - 1)
        int idx_cuenta = rand_r(&semilla) % NUM_CUENTAS;
        // Seleccionar una operación aleatoria: 0 = Depósito, 1 = Extracción, 2 = Consulta
        int tipo_op = rand_r(&semilla) % 3;
        // Monto aleatorio para operar (entre 10 y 200)
        double monto = 10.0 + (rand_r(&semilla) % 191);

        CuentaBancaria *cuenta = &cuentas[idx_cuenta];

        // Bloquear la cuenta antes de realizar cualquier lectura o modificación
        pthread_mutex_lock(&cuenta->mutex);

        printf("[Cajero %d] Iniciando operación en Cuenta N° %d...\n", id, cuenta->numero_cuenta);

        if (tipo_op == 0) {
            // DEPÓSITO
            cuenta->saldo += monto;
            printf("  -> OPERACIÓN: Depósito de $%.2f\n", monto);
            printf("  -> RESULTADO: Éxito. Saldo resultante: $%.2f\n", cuenta->saldo);
        } 
        else if (tipo_op == 1) {
            // EXTRACCIÓN (Con validación de saldo suficiente)
            printf("  -> OPERACIÓN: Intento de extracción de $%.2f\n", monto);
            if (cuenta->saldo >= monto) {
                cuenta->saldo -= monto;
                printf("  -> RESULTADO: Éxito. Saldo resultante: $%.2f\n", cuenta->saldo);
            } else {
                printf("  -> RESULTADO: RECHAZADO. Saldo insuficiente ($%.2f disponible).\n", cuenta->saldo);
            }
        } 
        else {
            // CONSULTA DE SALDO
            printf("  -> OPERACIÓN: Consulta de saldo\n");
            printf("  -> RESULTADO: Saldo actual: $%.2f\n", cuenta->saldo);
        }

        printf("----------------------------------------\n");

        // Desbloquear la cuenta una vez finalizada la operación
        pthread_mutex_unlock(&cuenta->mutex);

        // Pequeña pausa simulada entre operaciones para favorecer la concurrencia
        usleep((rand_r(&semilla) % 500 + 100) * 1000); 
    }

    pthread_exit(NULL);
}

int main() {
    srand(time(NULL));

    // 1. Inicializar las cuentas bancarias
    CuentaBancaria cuentas[NUM_CUENTAS];
    for (int i = 0; i < NUM_CUENTAS; i++) {
        cuentas[i].numero_cuenta = 1000 + i;
        cuentas[i].saldo = 500.0; // Saldo inicial base para pruebas
        if (pthread_mutex_init(&cuentas[i].mutex, NULL) != 0) {
            perror("Error al inicializar el mutex de la cuenta");
            return 1;
        }
    }

    printf("=== INICIANDO SIMULACIÓN DE CAJEROS AUTOMÁTICOS ===\n");
    printf("Cuentas disponibles: %d | Cajeros activos: %d\n", NUM_CUENTAS, NUM_CAJEROS);
    printf("===================================================\n\n");

    // 2. Crear los hilos de los cajeros
    pthread_t hilos_cajeros[NUM_CAJEROS];
    DatosCajero datos_cajeros[NUM_CAJEROS];

    for (int i = 0; i < NUM_CAJEROS; i++) {
        datos_cajeros[i].id_cajero = i + 1;
        datos_cajeros[i].cuentas = cuentas;

        if (pthread_create(&hilos_cajeros[i], NULL, simular_cajero, (void*)&datos_cajeros[i]) != 0) {
            perror("Error al crear el hilo del cajero");
            return 1;
        }
    }

    // 3. Esperar a que todos los cajeros terminen sus operaciones (pthread_join)
    for (int i = 0; i < NUM_CAJEROS; i++) {
        pthread_join(hilos_cajeros[i], NULL);
    }

    // 4. Mostrar el estado final consolidado y destruir mutexes
    printf("\n=== SIMULACIÓN FINALIZADA ===\n");
    printf("Estado final de las cuentas:\n");
    for (int i = 0; i < NUM_CUENTAS; i++) {
        printf("Cuenta N° %d - Saldo Final: $%.2f\n", cuentas[i].numero_cuenta, cuentas[i].saldo);
        pthread_mutex_destroy(&cuentas[i].mutex);
    }

    return 0;
}