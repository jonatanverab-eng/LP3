#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
 
#define N_PRODUCTORES   2
#define M_COCINEROS     1
#define R_REPARTIDORES  2
#define MAX_COLA        5
#define TOTAL_PEDIDOS   5
 
#define LIMITE_ESPERA   3 /* Maximo de segundos que un pedido puede esperar en la cola */

static const char* TIPOS_COMIDA[] = {"Pizza", "Hamburguesa", "Sushi", "Pasta", "Ensalada"};
#define N_TIPOS (int)(sizeof(TIPOS_COMIDA) / sizeof(TIPOS_COMIDA[0]))
 
typedef struct {
    int id;
    int tiempo_preparacion;
    const char* tipo_comida;
    time_t hora_creacion; /* Guardara el timestamp de cuando se creo el pedido */
} Pedido;
 
Pedido cola_pendientes[MAX_COLA];
int ini_pend = 0, fin_pend = 0;
 
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
        p.hora_creacion      = time(NULL);
 
        sem_wait(&sem_vacios_pend);
 
        pthread_mutex_lock(&mtx_pend);
        cola_pendientes[fin_pend] = p;
        fin_pend = (fin_pend + 1) % MAX_COLA;
        printf("[Productor %d] Pedido #%d generado - %s (prep: %ds)\n",
               id, p.id, p.tipo_comida, p.tiempo_preparacion);
        pthread_mutex_unlock(&mtx_pend);
 
        sem_post(&sem_llenos_pend);
 
        sleep((rand() % 2) + 1);
    }
 
    printf("[Productor %d] Finalizado.\n", id);
    return NULL;
}
 
/* ════════════════════════════════════════════════
   HILO COCINERO
   ════════════════════════════════════════════════ */
void* funcion_cocinero(void* arg) {
    int id = *(int*)arg;
    free(arg);
 
    while (1) {
        pthread_mutex_lock(&mtx_global);
        if (pedidos_cocinados >= TOTAL_PEDIDOS) {
            pthread_mutex_unlock(&mtx_global);
            sem_post(&sem_llenos_pend); /* Despierta a otro cocinero para que pueda salir del bucle */
            break;
        }
        pthread_mutex_unlock(&mtx_global);
 
        sem_wait(&sem_llenos_pend);
 
        pthread_mutex_lock(&mtx_global);
        if (pedidos_cocinados >= TOTAL_PEDIDOS) {
            pthread_mutex_unlock(&mtx_global);
            sem_post(&sem_llenos_pend); /* Despierta al siguiente en cascada */
            break;
        }
        pthread_mutex_unlock(&mtx_global);
 
        pthread_mutex_lock(&mtx_pend);
        if (ini_pend == fin_pend) {
            pthread_mutex_unlock(&mtx_pend);
            sem_post(&sem_llenos_pend);
            continue;
        }
 
        Pedido p = cola_pendientes[ini_pend];
        ini_pend = (ini_pend + 1) % MAX_COLA;
        pthread_mutex_unlock(&mtx_pend);
 
        sem_post(&sem_vacios_pend);
 
        /* Verificar si el pedido expiro esperando en la cola */
        time_t ahora = time(NULL);
        int tiempo_esperado = (int)(ahora - p.hora_creacion);
 
        if (tiempo_esperado > LIMITE_ESPERA) {
            pthread_mutex_lock(&mtx_global);
            pedidos_cocinados++;
            pedidos_entregados++; 
            printf("[Cocinero  %d] CANCELADO: El pedido #%d expiro (Espero %ds, Maximo: %ds). Se descarta.\n", 
                   id, p.id, tiempo_esperado, LIMITE_ESPERA);
            pthread_mutex_unlock(&mtx_global);
            
            sem_post(&sem_llenos_list); /* Despierta un repartidor para chequear el fin del sistema */
            continue;
        }
 
        printf("[Cocinero  %d] Tomo pedido #%d (%s)\n", id, p.id, p.tipo_comida);
 
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
   ════════════════════════════════════════════════ */
void* funcion_repartidor(void* arg) {
    int id = *(int*)arg;
    free(arg);
 
    while (1) {
        pthread_mutex_lock(&mtx_global);
        if (pedidos_entregados >= TOTAL_PEDIDOS) {
            pthread_mutex_unlock(&mtx_global);
            sem_post(&sem_llenos_list); /* Despierta a otro repartidor para que pueda salir */
            break;
        }
        pthread_mutex_unlock(&mtx_global);
 
        sem_wait(&sem_llenos_list);
 
        pthread_mutex_lock(&mtx_global);
        if (pedidos_entregados >= TOTAL_PEDIDOS) {
            pthread_mutex_unlock(&mtx_global);
            sem_post(&sem_llenos_list); /* Despierta al siguiente en cascada */
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
 
        sleep((rand() % 3) + 1);
 
        pthread_mutex_lock(&mtx_global);
        pedidos_entregados++;
        int mi_progreso = pedidos_entregados;
        printf("[Repartidor %d] Pedido #%d entregado con exito - (%d/%d)\n",
               id, p.id, mi_progreso, TOTAL_PEDIDOS);
        pthread_mutex_unlock(&mtx_global);
    }
 
    printf("[Repartidor %d] Finalizado.\n", id);
    return NULL;
}
 
/* ════════════════════════════════════════════════
   MAIN
   ════════════════════════════════════════════════ */
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
 
    printf("\n=== Simulacion completada. %d pedidos procesados/entregados. ===\n", pedidos_entregados);
    return 0;
}
