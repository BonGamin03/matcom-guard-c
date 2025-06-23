#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("Ejecutando proceso con fuga de memoria...\n");
    system("xterm -e 'tail /dev/zero' &");
    printf("Proceso lanzado. Espera unos segundos y verifica si el sistema lanza alerta por uso de RAM.\n");
    sleep(15); // Da tiempo para que lo detecte
    return 0;
}