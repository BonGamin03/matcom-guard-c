#include <stdlib.h>
#include <stdio.h>

int main() {
    printf("Ejecutando proceso legítimo: gcc\n");
    system("gcc --version > /dev/null");  // Simula uso normal de gcc
    printf("Proceso legítimo ejecutado. Verifica que no se generó ninguna alerta.\n");
    return 0;
}