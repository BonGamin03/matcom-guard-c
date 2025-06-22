#include <stdio.h>
#include "util.h"
#include <time.h>
#include <stdlib.h>

#define ALERT_FILE "Report/alertas_guard.txt"


/*
void Write_Alert(const char * message) {
    FILE *alert = fopen(ALERT_FILE, "a");
    if (alert) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char fecha[32];
        strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tm_info);
        // Puedes separar tipo y detalle si lo deseas, aquí solo ejemplo:
        fprintf(alert, "║ %-20s │ %-12s │ %-40s ║\n", fecha, "ALERTA", message);
        fclose(alert);
    }
}

void inicializar_alertas_guard() {
    FILE *f = fopen("Report/alertas_guard.txt", "r");
    int vacio = 1;
    if (f) {
        fseek(f, 0, SEEK_END);
        vacio = (ftell(f) == 0);
        fclose(f);
    }
    if (vacio) {
        f = fopen("Report/alertas_guard.txt", "w");
        if (f) {
            fprintf(f,
"╔══════════════════════════════════════════════════════════════════════════════╗\n"
"║                 🛡️  MATCOM GUARD - ALERTAS EN TIEMPO REAL  🛡️               ║\n"
"╠══════════════════════════════════════════════════════════════════════════════╣\n"
"║  Fecha y hora        │ Tipo de alerta │ Detalle                             ║\n"
"╠══════════════════════╬════════════════╬══════════════════════════════════════╣\n"
            );
            fclose(f);
        }
    }
}
*/


//Tabla y mensage con Colores

void Write_Alert(const char * message) {
    FILE *alert = fopen(ALERT_FILE, "a");
    if (alert) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char fecha[32];
        strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tm_info);
        // Color verde para la fila de alerta
        fprintf(alert, "\033[0;32m║ %-20s │ %-12s │ %-40s ║\033[0m\n", fecha, "ALERTA", message);
        fclose(alert);
    }
}

void inicializar_alertas_guard() {

    // Crear archivo de alertas vacío si no existe
    system("touch Report/alertas_guard.txt");
    // Configurar permisos para que sea accesible
    system("chmod 777 /Report/alertas_guard.txt");

    // Verificar si el archivo está vacío y escribir encabezado si es necesario
    FILE *f = fopen("Report/alertas_guard.txt", "r");
    int vacio = 1;
    if (f) {
        fseek(f, 0, SEEK_END);
        vacio = (ftell(f) == 0);
        fclose(f);
    }
    if (vacio) {
        f = fopen("Report/alertas_guard.txt", "w");
        if (f) {
            // Códigos ANSI para color: 36 = cyan, 33 = amarillo, 32 = verde, 31 = rojo, 35 = magenta
            fprintf(f,
"\033[1;36m╔══════════════════════════════════════════════════════════════════════════════╗\n"
"║                 🛡️  \033[1;33mMATCOM GUARD - ALERTAS EN TIEMPO REAL\033[1;36m  🛡️               ║\n"
"╠══════════════════════════════════════════════════════════════════════════════╣\n"
"║  \033[1;32mFecha y hora        │ Tipo de alerta │ Detalle\033[1;36m                             ║\n"
"╠══════════════════════╬════════════════╬══════════════════════════════════════╣\033[0m\n"
            );
            fclose(f);
        }
    }
}