
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#define MAX_REPORT 4096

// Variables globales para almacenar los resultados de los escaneos
char fs_report[MAX_REPORT] = "";
char mem_report[MAX_REPORT] = "";
char port_report[MAX_REPORT] = "";

// Mutex para sincronizar acceso a los reportes
pthread_mutex_t report_mutex = PTHREAD_MUTEX_INITIALIZER;


void *scanFilesystem(void *arg) {
    
    // Compilar el monitor USB
    system("gcc -o Scanners/USB/usb_scanner.c -lpthread");
    
    // Ejecutar el monitor USB por un tiempo limitado (timeout)
    FILE *fp = popen("timeout 10s ./Scanners/USB/usb_scanner", "r");
    if (!fp) {
        pthread_mutex_lock(&report_mutex);
        snprintf(fs_report, sizeof(fs_report), "Error: No se pudo ejecutar el monitor USB\n");
        pthread_mutex_unlock(&report_mutex);
        pthread_exit(NULL);
    }

    char buffer[256];
    char temp[MAX_REPORT] = "";
    while (fgets(buffer, sizeof(buffer), fp)) {
        strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
    }
    pclose(fp);

    // Si no hay salida del monitor, hacer un escaneo básico como respaldo
    if (strlen(temp) == 0) {
        FILE *fp_backup = popen("ls -lah /media/ /mnt/ 2>/dev/null", "r");
        if (fp_backup) {
            strcat(temp, "=== DISPOSITIVOS USB MONTADOS ===\n");
            while (fgets(buffer, sizeof(buffer), fp_backup)) {
                strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
            }
            pclose(fp_backup);
        }
        
        // También mostrar información de lsusb si está disponible
        FILE *fp_usb = popen("lsusb 2>/dev/null", "r");
        if (fp_usb) {
            strcat(temp, "\n=== DISPOSITIVOS USB CONECTADOS ===\n");
            while (fgets(buffer, sizeof(buffer), fp_usb)) {
                strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
            }
            pclose(fp_usb);
        }
    }

    pthread_mutex_lock(&report_mutex);
    snprintf(fs_report, sizeof(fs_report), "%s", temp);
    pthread_mutex_unlock(&report_mutex);

    // Crear directorio de reportes si no existe
    system("mkdir -p Report");
    
    FILE *out = fopen("Report/reporte_sistema_archivos.txt", "w");
    if (out) {
        fputs(temp, out);
        fclose(out);
    }
    pthread_exit(NULL);
}

/*
// Función para escanear sistema de archivos
void *scanFilesystem(void *arg) {

    // Compilar el escáner de USB
    system("gcc -o Scanners/USB/usb_scanner Scanners/USB/usb_scanner.c -lpthread");

    while (1) {
        char cmd[256];
        char buffer[4096];
        char temp[MAX_REPORT] = "";

        // Ejecutar el escáner de USB
        snprintf(cmd, sizeof(cmd), "./Scanners/USB/usb_scanner");
        FILE *fp = popen(cmd, "r");
        if (!fp) {
            fprintf(stderr, "Error: no se pudo ejecutar el comando: %s\n", cmd);
            sleep(10);
            continue;
        }

        while (fgets(buffer, sizeof(buffer), fp)) {
            strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
        }
        pclose(fp);

        pthread_mutex_lock(&report_mutex);
        snprintf(fs_report, sizeof(fs_report), "%s", temp);
        pthread_mutex_unlock(&report_mutex);

        FILE *out = fopen("Report/reporte_sistema_archivos.txt", "w");
        if (out) {
            fputs(temp, out);
            fclose(out);
        } else {
            fprintf(stderr, "Error: no se pudo abrir el archivo de reporte.\n");
            sleep(10);
            continue;
        }
        sleep(10); // Espera 10 segundos antes de volver a escanear
    }
    pthread_exit(NULL);
}
*/

// Función para escanear memoria (usando el comando 'free' en Linux)
void *scanMemory(void *arg) {

    // Compilar el escáner de memoria
    system("gcc -o Scanners/Proccess/proccess_monitor Scanners/Proccess/process_monitor.c -lpthread");

    // Asignar capacidades necesarias para el escáner de memoria desde C
    //system("setcap cap_sys_ptrace=eip Scanners/Proccess/proccess_monitor");
        
    while (1) {
        char cmd[256];
        char buffer[4096];
        char temp[MAX_REPORT] = "";

        // Ejecutar el escáner de memoria
        snprintf(cmd, sizeof(cmd), "./Scanners/Proccess/proccess_monitor");
        FILE *fp = popen(cmd, "r");
        if (!fp) {
            fprintf(stderr, "Error: no se pudo ejecutar el comando: %s\n", cmd);
            sleep(10);
            continue;
        }

        while (fgets(buffer, sizeof(buffer), fp)) {
            strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
        }
        pclose(fp);

        pthread_mutex_lock(&report_mutex);
        snprintf(mem_report, sizeof(mem_report), "%s", temp);
        pthread_mutex_unlock(&report_mutex);

        FILE *out = fopen("Report/reporte_memoria.txt", "w");
        if (out) {
            fputs(temp, out);
            fclose(out);
        }
        else {
            fprintf(stderr, "Error: no se pudo abrir el archivo de reporte.\n");
            sleep(10);
            continue;
        }
        sleep(10);// Espera 10 segundos antes de volver a escanear
    }
    pthread_exit(NULL);
}

// Función para escanear puertos
void *scanPorts(void *arg) {

     // Compilar el escáner de puertos
    system("gcc -o Scanners/Ports/port_scanner Scanners/Ports/port_scanner.c -lpthread");        

    // Asignar capacidades necesarias para el escáner de puertos desde C
    //system("setcap cap_net_raw,cap_net_bind_service=eip Scanners/Ports/port_scanner");

    while (1) {
        char cmd[256];
        char ip[] = "127.0.0.1";
        int start_port = 1, end_port = 65535;
        char buffer[4096];
        char temp[MAX_REPORT] = "";

        // Ejecutar el escáner de puertos
        snprintf(cmd, sizeof(cmd), "./Scanners/Ports/port_scanner %s %d %d", ip, start_port, end_port);
        FILE *fp = popen(cmd, "r");
        if (!fp) {
            fprintf(stderr, "Error: no se pudo ejecutar el comando: %s\n", cmd);
            sleep(10);
            continue;
        }
        
        while (fgets(buffer, sizeof(buffer), fp)) {
            strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
        }
        pclose(fp);

        pthread_mutex_lock(&report_mutex);
        snprintf(port_report, sizeof(port_report), "%s", temp);
        pthread_mutex_unlock(&report_mutex);

        FILE *out = fopen("Report/reporte_puertos.txt", "w");
        if (out) {
            fputs(temp, out);
            fclose(out);
        }
        else {
            fprintf(stderr, "Error: no se pudo abrir el archivo de reporte.\n");
            sleep(10);
            continue;
        }
        sleep(10);// Espera 10 segundos antes de volver a escanear
    }
    pthread_exit(NULL);
}
// Función para imprimir los reportes en consola
void print_fs_report() {
    pthread_mutex_lock(&report_mutex);
    //printf("\033[1;33m------------------------------------------------------------------------\033[0m\n");
    printf("\n\033[1;32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ REPORTE USB ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");
    printf("\n%s\n", fs_report);
    pthread_mutex_unlock(&report_mutex);
    printf("\033[1;33m------------------------------------------------------------------------\033[0m\n");
}
void print_mem_report() {
    pthread_mutex_lock(&report_mutex);
    //printf("\033[1;33m------------------------------------------------------------------------\033[0m\n");
    printf("\n\033[1;32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ REPORTE MEMORIA ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");
    printf("\n%s\n", mem_report);
    pthread_mutex_unlock(&report_mutex);
    printf("\033[1;33m------------------------------------------------------------------------\033[0m\n\n");
}
void print_port_report() {
    pthread_mutex_lock(&report_mutex);
    //printf("\033[1;33m------------------------------------------------------------------------\033[0m\n");
     printf("\n\033[1;32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ REPORTE PUERTOS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");
    printf("\n%s\n", port_report);
    pthread_mutex_unlock(&report_mutex);
    printf("\033[1;33m------------------------------------------------------------------------\033[0m\n");
}

void GeneratePDF() {
    // Verificar si los reportes existen
    if (access("Report/reporte_sistema_archivos.txt", F_OK) == -1 ||
        access("Report/reporte_memoria.txt", F_OK) == -1 ||
        access("Report/reporte_puertos.txt", F_OK) == -1) {
        printf("Generando reportes antes de exportar a PDF...\n");
        scanFilesystem(NULL);
        scanMemory(NULL);
        scanPorts( 0); // No interactivo
    }

    // Generar el PDF usando enscript y ps2pdf
    system("enscript -p Report/reporte.ps Report/reporte_sistema_archivos.txt Report/reporte_memoria.txt Report/reporte_puertos.txt");
    system("ps2pdf Report/reporte.ps Report/reporte.pdf");
    //verificar si el PDF se generó correctamente
    if (access("Report/reporte.pdf", F_OK) == -1) {
        printf("Error al generar el PDF. Asegúrese de tener 'enscript' y 'ps2pdf' instalados.\n");
        return;
    }
    // Mostrar mensaje de éxito
    printf("\n\033[1;32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ REPORTE EXPORTADO A PDF ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");
    printf("🔍 Reporte exportado a PDF en: 📄Report/reporte.pdf\n");
}

void abrir_pdf_linux(const char *nombre_pdf) {
    char ruta_pdf[256];
    snprintf(ruta_pdf, sizeof(ruta_pdf), "Report/%s", nombre_pdf);

    if (access(ruta_pdf, F_OK) == -1) {
        printf("El PDF no existe. Genérelo primero.\n");
        return;
    }

    if (system("which xdg-open >/dev/null") == 0) {
        char comando[512];
        snprintf(comando, sizeof(comando), "xdg-open \"%s\" &", ruta_pdf);
        system(comando);
    } else {
        printf("Instale xdg-open para visualizar PDFs:\n");
        printf("sudo apt install xdg-utils\n");
    }
}

int main() {
    int option;
    pthread_t fs_tid, mem_tid, port_tid;
    // Inicializar mutex
    pthread_mutex_init(&report_mutex, NULL);

    // Crear carpeta de reportes si no existe
    system("mkdir -p Report");

    // Lanzar los escaneos en hilos (solo FS y Memoria)
    pthread_create(&fs_tid, NULL, scanFilesystem, NULL);
    pthread_create(&mem_tid, NULL, scanMemory, NULL);
    pthread_create(&port_tid, NULL, scanPorts, NULL);
    //pthread_create(&port_tid, NULL, (void *(*)(void *))scanPorts, (void *)0);
    
    // Esperar a que terminen los escaneos antes de mostrar menú
    //pthread_join(fs_tid, NULL);
    //pthread_join(mem_tid, NULL);
    //pthread_join(port_tid, NULL);

    // Ciclo infinito de escaneo en segundo plano
    while (1) {
       
        // Mostrar menú de usuario
        printf("\033[1;33m========================================\033[0m\n");
        printf("\n\033[1;36m=== 🛡️ Gran Salón del Trono 🛡️ ===\033[0m\n");
        printf("\033[1;33m1.\033[0m \033[0;32mEscanearreporte de sistema de archivos\033[0m\n");
        printf("\033[1;33m2.\033[0m \033[0;32mEscanearreporte de memoria\033[0m\n");
        printf("\033[1;33m3.\033[0m \033[0;32mEscanearreporte de puertos\033[0m\n");
        printf("\033[1;33m4.\033[0m \033[0;32mMostrar todos los reportes\033[0m\n");
        printf("\033[1;33m5.\033[0m \033[0;34mExportar reportes a PDF\033[0m\n");
        printf("\033[1;33m6.\033[0m \033[0;34mAbrir PDF de reportes\033[0m\n");
        printf("\033[1;33m7.\033[0m \033[1;31mSalir\033[0m\n");
        printf("\n");
        printf("\033[1;33m========================================\033[0m\n");
        printf("\033[1;35mSelecciona una opción:\033[0m ");
        scanf("%d", &option);
        getchar(); // Limpiar salto de línea pendiente

        switch (option) {
            case 1:
                print_fs_report();
                break;
            case 2:
                print_mem_report();
                break;
            case 3:
               // printf("\n\033[1;32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ESCANEO DE PUERTOS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n\n");
                //scanPorts(1); // Interactivo
                //print_port_report();
                //printf("\033[1;33m-----------------------------------------------------------------------------\033[0m\n");
                print_port_report();
                break;
            case 4:
                print_fs_report();
                print_mem_report();
                print_port_report();
                printf("\n--- Todos los reportes mostrados ---\n");
                break;
            case 5:
                GeneratePDF();
                break;
            case 6:
                abrir_pdf_linux("reporte.pdf");
                break;
            case 7:
                printf("Saliendo del programa...\n");
                pthread_mutex_destroy(&report_mutex);
                return 0;
            default:
                printf("Opción inválida. Inténtalo nuevamente.\n");
        }
        printf("\npresione enter para continuar...");
        getchar();// Limpiar salto de línea pendiente
        //getchar(); // Esperar a que el usuario presione enter
        system("clear");
    }
    return 0;
}

/*
if (!device->mount_point || !event->name) {
    fprintf(stderr, "Error: mount_point o name son NULL\n");
    return NULL; // o maneja el error adecuadamente
}
*/