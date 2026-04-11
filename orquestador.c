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
    
   
