#include <stdio.h>
#include "util.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/file.h>  // Para flock()
#include <unistd.h>

#define ALERT_FILE "Report/alertas_guard.txt"
pthread_mutex_t mutex_alertas;

void inicializar_mutex_alertas() {
    pthread_mutex_init(&mutex_alertas, NULL);
}


void destruir_mutex_alertas() {
    pthread_mutex_destroy(&mutex_alertas);
}

void destruir_alertas_guard(void) {
    // Eliminar el archivo de alertas
    if (remove(ALERT_FILE) == 0) {
        printf("✅ Archivo de alertas eliminado correctamente.\n");
    } else {
        perror("❌ Error al eliminar el archivo de alertas");
    }
}

void limpiar_usb_baseline(void) {
    if (remove("Scanners/USB/usb_baseline.db") == 0) {
        printf("✅ Base de datos del USB eliminada correctamente.\n");
    } else {
        perror("❌ Error al eliminar la base de datos del USB");
    }
}

//Tabla y mensage con Colores
void Write_Alert(const char* type_scanner, const char * message) {
    pthread_mutex_lock(&mutex_alertas);

    // 1. Revisar si el mensaje ya existe en el archivo
    int exists = 0;
    FILE *alert = fopen(ALERT_FILE, "r");
    if (alert) {
        char linea[1024];
        while (fgets(linea, sizeof(linea), alert)) {
            if (strstr(linea, type_scanner) && strstr(linea, message)) {
                exists = 1;
                break;
            }
        }
        fclose(alert);
    }

    // 2. Si no existe, escribir la alerta
    if (!exists) {
        alert = fopen(ALERT_FILE, "a");
        if (alert) {
            int fd = fileno(alert);
            flock(fd, LOCK_EX);

            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            char fecha[32];
            strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tm_info);

            // Delimitadores cyan, fecha verde, tipo magenta, detalle amarillo
            fprintf(alert, "\033[1;36m║\033[0m \033[1;32m%-20s\033[0m \033[1;36m│\033[0m \033[1;35m%-12s\033[0m \033[1;36m│\033[0m \033[1;33m%-40s\033[0m \033[1;36m║\033[0m\n",
                fecha, type_scanner, message);

            fflush(alert);
            flock(fd, LOCK_UN);
            fclose(alert);
        } else {
            fprintf(stderr, "❌ Error al abrir el archivo alertas_guard.txt: %s\n", strerror(errno));
        }
    }

    pthread_mutex_unlock(&mutex_alertas);
}

/*
void Write_Alert(const char* type_scanner,const char * message) {

    pthread_mutex_lock(&mutex_alertas);

    FILE *alert = fopen(ALERT_FILE, "a");
    if (alert) {
        // Bloqueo de archivo a nivel de sistema
        int fd = fileno(alert);
        flock(fd, LOCK_EX);

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char fecha[32];
        strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", tm_info);
        // Color verde para la fila de alerta
        fprintf(alert, "\033[0;32m║ %-20s │ %-12s │ %-40s ║\033[0m\n", fecha, type_scanner, message);
        fflush(alert);  // Forzar escritura inmediata
        flock(fd, LOCK_UN);  // Desbloquear el archivo
        fclose(alert);
    }
    else
    {
        fprintf(stderr, "❌ Error al abrir el archivo alertas_guard.txt: %s\n", strerror(errno));
    }

    pthread_mutex_unlock(&mutex_alertas);
}
*/

void inicializar_alertas_guard() {

    // Crear archivo de alertas vacío si no existe
    system("touch Report/alertas_guard.txt");
    // Configurar permisos para que sea accesible
    system("chmod 777 Report/alertas_guard.txt");

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
            // Códigos ANSI para color: 36 = cyan, 33 = amarillo, 32 = verde, 37 = blanco, 35 = magenta
            fprintf(f,
"\033[1;36m╔══════════════════════════════════════════════════════════════════════════════╗\n"
"║                 🛡️  \033[1;37mMATCOM GUARD - ALERTAS EN TIEMPO REAL\033[1;36m  🛡️                  ║\n"
"╠══════════════════════════════════════════════════════════════════════════════╣\n"
"║  \033[1;37mFecha y hora        │ Tipo de alerta │ Detalle\033[1;36m                              ║\n"
"╠══════════════════════╬════════════════╬══════════════════════════════════════╣\033[0m\n"
            );
            fclose(f);
        }
    }
}