#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Lanzando proceso para ser eliminado en xterm...\n");

     system("xterm -e 'bash -c \"while true; do :; done\"' &");
    sleep(2); // Dar tiempo a que el proceso inicie

    printf("Obteniendo PID del proceso 'sleep'...\n");
    system("pgrep -f 'while true' > pid.txt");

    FILE *f = fopen("pid.txt", "r");
    int pid;
    if (f && fscanf(f, "%d", &pid) == 1) {
        printf("Matando proceso con PID %d\n", pid);
        sleep(30);
        char cmd[64];
        sprintf(cmd, "kill %d", pid);
        system(cmd);
        printf("Proceso terminado. Verifica que el sistema ya no lo monitorea.\n");
    } else {
        printf("No se encontró el PID.\n");
    }
    if (f) fclose(f);
    return 0;
}