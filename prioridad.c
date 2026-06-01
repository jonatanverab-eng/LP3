#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
 
#define N_PRODUCTORES   2
#define M_COCINEROS     3
#define R_REPARTIDORES  2
#define MAX_COLA        5
#define TOTAL_PEDIDOS   10
 
static const char* TIPOS_COMIDA[] = {"Pizza", "Hamburguesa", "Sushi", "Pasta", "Ensalada"};
#define N_TIPOS (int)(sizeof(TIPOS_COMIDA) / sizeof(TIPOS_COMIDA[0]))
 
typedef struct {
    int id;
    int tiempo_preparacion;
    const char* tipo_comida;
    int es_vip; /* 1 si es VIP, 0 si es normal */
} Pedido;
 
Pedido cola_pendientes[MAX_COLA];
int ini_pend = 0, fin_pend = 0;
int count_pend = 0; /* Contador para habilitar la busqueda no lineal en el buffer circular */
 
Pedido cola_listos[MAX_COLA];
int ini_list = 0, fin_list = 0;
 
int pedidos_generados  = 0;
int pedidos_cocinados  = 0;
int pedidos_entregados = 0;
 
pthread_mutex_t mtx_pend;
pthread_mutex_t mtx_list;
pthread_mutex_t mtx_global;
 
sem_t sem_vacios_pend;
sem_t sem_llenos_pend;
 
sem_t sem_vacios_list;
sem_t sem_llenos_list;
 
/* ════════════════════════════════════════════════
   HILO PRODUCTOR
   Genera hasta TOTAL_PEDIDOS pedidos en total
   entre todos los productores.
   ════════════════════════════════════════════════ */
void* funcion_productor(void* arg) {
    int id = *(int*)arg;
    free(arg);
 
    while (1) {
        pthread_mutex_lock(&mtx_global);
        if (pedidos_generados >= TOTAL_PEDIDOS) {
            pthread_mutex_unlock(&mtx_global);
            break;
        }
        pedidos_generados++;
        int mi_id = pedidos_generados;
        pthread_mutex_unlock(&mtx_global);
 
        Pedido p;
        p.id                 = mi_id;
        p.tiempo_preparacion = (rand() % 3) + 1;
        p.tipo_comida        = TIPOS_COMIDA[rand() % N_TIPOS];
        p.es_vip             = (rand() % 4 == 0); /* 25% de probabilidad de ser VIP */
 
        sem_wait(&sem_vacios_pend);
 
        pthread_mutex_lock(&mtx_pend);
        cola_pendientes[fin_pend] = p;
        fin_pend = (fin_pend + 1) % MAX_COLA;
        count_pend++;
        
        if (p.es_vip) {
            printf("[Productor %d] ATENCION: ¡PEDIDO VIP #%d GENERADO! - %s (prep: %ds)\n",
                   id, p.id, p.tipo_comida, p.tiempo_preparacion);
        } else {
            printf("[Productor %d] Pedido #%d generado - %s (prep: %ds)\n",
                   id, p.id, p.tipo_comida, p.tiempo_preparacion);
        }
        pthread_mutex_unlock(&mtx_pend);
 
        sem_post(&sem_llenos_pend);
 
        sleep((rand() % 2) + 1);
    }
 
    printf("[Productor %d] Finalizado.\n", id);
    return NULL;
}
 
/* ════════════════════════════════════════════════
   HILO COCINERO
   Cocina pedidos hasta que se hayan cocinado
   todos (TOTAL_PEDIDOS en total entre todos).
   ════════════════════════════════════════════════ */
void* funcion_cocinero(void* arg) {
    int id = *(int*)arg;
    free(arg);
 
    while (1) {
        pthread_mutex_lock(&mtx_global);
        if (pedidos_cocinados >= TOTAL_PEDIDOS) {
            pthread_mutex_unlock(&mtx_global);
            break;
        }
        pthread_mutex_unlock(&mtx_global);
 
        sem_wait(&sem_llenos_pend);
 
        pthread_mutex_lock(&mtx_global);
        if (pedidos_cocinados >= TOTAL_PEDIDOS) {
            pthread_mutex_unlock(&mtx_global);
            sem_post(&sem_llenos_pend);
            break;
        }
        pthread_mutex_unlock(&mtx_global);
 
        pthread_mutex_lock(&mtx_pend);
        if (ini_pend == fin_pend) {
            pthread_mutex_unlock(&mtx_pend);
            sem_post(&sem_llenos_pend);
            continue;
        }
 
        /* Busqueda de un pedido VIP dentro del buffer circular */
        int idx_elegido = -1;
        int idx_actual = ini_pend;
        for (int c = 0; c < count_pend; c++) {
            if (cola_pendientes[idx_actual].es_vip) {
                idx_elegido = idx_actual;
                break; /* Detiene al encontrar el primer VIP prioritario */
            }
            idx_actual = (idx_actual + 1) % MAX_COLA;
        }
 
        Pedido p;
        if (idx_elegido != -1) {
            p = cola_pendientes[idx_elegido];
            /* Desplazar elementos en el buffer circular para acomodar la extraccion */
            int aux = idx_elegido;
            while (aux != ini_pend) {
                int anterior = (aux - 1 + MAX_COLA) % MAX_COLA;
                cola_pendientes[aux] = cola_pendientes[anterior];
                aux = anterior;
            }
            ini_pend = (ini_pend + 1) % MAX_COLA;
            count_pend--;
            printf("[Cocinero  %d] PRIORIDAD TOTAL: Tomo pedido VIP #%d (%s)\n", id, p.id, p.tipo_comida);
        } else {
            /* Comportamiento original si no hay pedidos VIP */
            p = cola_pendientes[ini_pend];
            ini_pend = (ini_pend + 1) % MAX_COLA;
            count_pend--;
            printf("[Cocinero  %d] Tomo pedido #%d (%s)\n", id, p.id, p.tipo_comida);
        }
        pthread_mutex_unlock(&mtx_pend);
 
        sem_post(&sem_vacios_pend);
 
        pthread_mutex_lock(&mtx_global);
        pedidos_cocinados++;
        pthread_mutex_unlock(&mtx_global);
 
        sleep(p.tiempo_preparacion);
 
        sem_wait(&sem_vacios_list);
 
        pthread_mutex_lock(&mtx_list);
        cola_listos[fin_list] = p;
        fin_list = (fin_list + 1) % MAX_COLA;
        printf("[Cocinero  %d] Pedido #%d listo -> zona de entrega\n", id, p.id);
        pthread_mutex_unlock(&mtx_list);
 
        sem_post(&sem_llenos_list);
    }
 
    printf("[Cocinero  %d] Finalizado.\n", id);
    return NULL;
}
 
/* ════════════════════════════════════════════════
   HILO REPARTIDOR
   Entrega pedidos hasta completar TOTAL_PEDIDOS.
   ════════════════════════════════════════════════ */
void* funcion_repartidor(void* arg) {
    int id = *(int*)arg;
    free(arg);
 
    while (1) {
        pthread_mutex_lock(&mtx_global);
        if (pedidos_entregados >= TOTAL_PEDIDOS) {
            pthread_mutex_unlock(&mtx_global);
            break;
        }
        pthread_mutex_unlock(&mtx_global);
 
        sem_wait(&sem_llenos_list);
 
        pthread_mutex_lock(&mtx_global);
        if (pedidos_entregados >= TOTAL_PEDIDOS) {
            pthread_mutex_unlock(&mtx_global);
            sem_post(&sem_llenos_list);
            break;
        }
        pthread_mutex_unlock(&mtx_global);
 
        pthread_mutex_lock(&mtx_list);
        if (ini_list == fin_list) {
            pthread_mutex_unlock(&mtx_list);
            sem_post(&sem_llenos_list);
            continue;
        }
 
        Pedido p = cola_listos[ini_list];
        ini_list = (ini_list + 1) % MAX_COLA;
        printf("[Repartidor %d] Retiro pedido #%d (%s) para entrega\n",
               id, p.id, p.tipo_comida);
        pthread_mutex_unlock(&mtx_list);
 
        sem_post(&sem_vacios_list);
 
        pthread_mutex_lock(&mtx_global);
        pedidos_entregados++;
        int mi_progreso = pedidos_entregados;
        pthread_mutex_unlock(&mtx_global);
 
        sleep((rand() % 3) + 1);
 
        pthread_mutex_lock(&mtx_global);
        printf("[Repartidor %d] Pedido #%d entregado con exito - (%d/%d)\n",
               id, p.id, mi_progreso, TOTAL_PEDIDOS);
        pthread_mutex_unlock(&mtx_global);
    }
 
    printf("[Repartidor %d] Finalizado.\n", id);
    return NULL;
}
 
int main(void) {
    srand((unsigned)time(NULL));
 
    pthread_t productores [N_PRODUCTORES];
    pthread_t cocineros   [M_COCINEROS];
    pthread_t repartidores[R_REPARTIDORES];
 
    pthread_mutex_init(&mtx_pend,   NULL);
    pthread_mutex_init(&mtx_list,   NULL);
    pthread_mutex_init(&mtx_global, NULL);
 
    sem_init(&sem_vacios_pend, 0, MAX_COLA);
    sem_init(&sem_llenos_pend, 0, 0);
    sem_init(&sem_vacios_list, 0, MAX_COLA);
    sem_init(&sem_llenos_list, 0, 0);
 
    printf("=== Sistema de Delivery iniciado ===\n");
    printf("Productores: %d | Cocineros: %d | Repartidores: %d | Pedidos: %d\n\n",
           N_PRODUCTORES, M_COCINEROS, R_REPARTIDORES, TOTAL_PEDIDOS);
 
    for (int i = 0; i < N_PRODUCTORES; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&productores[i], NULL, funcion_productor, id);
    }
    for (int i = 0; i < M_COCINEROS; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&cocineros[i], NULL, funcion_cocinero, id);
    }
    for (int i = 0; i < R_REPARTIDORES; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&repartidores[i], NULL, funcion_repartidor, id);
    }
 
    for (int i = 0; i < N_PRODUCTORES;  i++) pthread_join(productores[i],  NULL);
    for (int i = 0; i < M_COCINEROS;    i++) pthread_join(cocineros[i],    NULL);
    for (int i = 0; i < R_REPARTIDORES; i++) pthread_join(repartidores[i], NULL);
 
    pthread_mutex_destroy(&mtx_pend);
    pthread_mutex_destroy(&mtx_list);
    pthread_mutex_destroy(&mtx_global);
    sem_destroy(&sem_vacios_pend);
    sem_destroy(&sem_llenos_pend);
    sem_destroy(&sem_vacios_list);
    sem_destroy(&sem_llenos_list);
 
    printf("\n=== Simulacion completada. %d pedidos entregados. ===\n", pedidos_entregados);
    return 0;
}
