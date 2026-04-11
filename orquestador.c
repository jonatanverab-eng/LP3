#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

void mensaje_final(void){
    printf("\n[PADRE] El proceso padre esta finalizando correctamente.\n");
}

int main(int argc, char *argv[]){
    if (argc < 2){
        printf("Falta insertar el numero N. Asi: %s <N>\n", argv[0]);
        return 1;
    }
    
    int n= atoi(argv[1]);
    if (n<=0){
        printf("Error: El valor de N debe ser un numero positivo.\n");
        return 1;
    }
    // Registro de la función de finalización
        atexit(mensaje_final);
        
        pid_t pids[n];
        
        for(int i=0;i<n;i++){
            // Creación de procesos hijos
            pids[i]= fork();
            
            if(pids[i]<0){
                perror("Error al crear el proceso hijo");
                exit(1);
            }
            
            if(pids[i]==0){
                // Generación de semilla para números aleatorios
                srand(getpid());
                int tiempo = (rand() % 5) + 1;
                
                printf("[HIJO %d] Mi PID es %d y el de mi padre es %d. Dormire %d segundos.\n",i+1,getpid(),getppid(),tiempo);
                
                sleep(tiempo);
                
                printf("[HIJO %d] Termine mi tarea.\n",i+1);
                
                // Finalización del proceso hijo
                _exit(i+1);
            }
        }
    
   
