
//Comando para compilarlo correctamente:
//gcc GST.c Scanners/util.c -o GST -lpthread   //normal
// Comando para compilarlo con debug:
//gcc GST.c Scanners/util.c -o GST -lpthread -g  // para debug
//gdb ./GST
//run
//bt //para mostrar errores

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "Scanners/util.h"
#define MAX_REPORT 4096
#define ALERT_FILE "Report/alertas_guard.txt"


// Variables globales para almacenar los resultados de los escaneos
char fs_report[MAX_REPORT] = "";
char mem_report[MAX_REPORT] = "";
char port_report[MAX_REPORT] = "";

// Mutex para sincronizar acceso a los reportes
pthread_mutex_t report_mutex = PTHREAD_MUTEX_INITIALIZER;

// Variable para controlar el estado de los hilos
volatile int keep_running = 1;

// Función auxiliar para verificar si un archivo existe
int file_exists(const char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

// Función auxiliar para compilar archivos de forma segura
int compile_if_needed(const char *source_path, const char *executable_path, const char *compile_flags) {
    // Verificar si el archivo fuente existe
    if (!file_exists(source_path)) {
        printf("❌ Error: El archivo fuente %s no existe\n", source_path);
        return -1;
    }
    
    // Verificar si necesita compilación (si el ejecutable no existe o es más viejo)
    struct stat source_stat, exec_stat;
    int need_compile = 1;
    
    if (stat(source_path, &source_stat) == 0 && stat(executable_path, &exec_stat) == 0) {
        // Si el ejecutable es más nuevo que el fuente, no compilar
        if (exec_stat.st_mtime >= source_stat.st_mtime) {
            need_compile = 0;
        }
    }
    
    if (need_compile) {
        char compile_cmd[512];
        // Formato común: gcc Scanners/util.c <source_path> -o <executable_path> <compile_flags>
        snprintf(compile_cmd, sizeof(compile_cmd),
            "gcc Scanners/util.c %s -o %s %s 2>/dev/null",
            source_path, executable_path, compile_flags ? compile_flags : "");

        printf("🔨 Compilando %s...\n", source_path);
        int result = system(compile_cmd);
        if (result != 0) {
            printf("❌ Error al compilar %s\n", source_path);
            return -1;
        }
        printf("✅ Compilación exitosa: %s\n", executable_path);
    }
    
    return 0;
}

// Función para escanear sistema de archivos
void *scanFilesystem(void *arg) {
    const char *source_file = "Scanners/USB/usb_scanner.c";
    const char *executable = "./Scanners/USB/usb_scanner";
    
    while (keep_running) {
        
        char buffer[4096];
        char temp[MAX_REPORT] = "";
        
        // Crear directorio si no existe
        system("mkdir -p Scanners/USB");
        
        // Compilar si es necesario
        if (compile_if_needed(source_file, executable, "-lpthread") != 0) {
            snprintf(temp, sizeof(temp), "❌ Error: No se pudo compilar o ejecutar el escáner USB\n");
        } else {
            // Ejecutar el escáner
            FILE *scanner = popen(executable, "r");
            if (scanner) {
                while (fgets(buffer, sizeof(buffer), scanner) && strlen(temp) < MAX_REPORT - 100) {
                    strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
                }
                pclose(scanner);
            } else {
                snprintf(temp, sizeof(temp), "❌ Error: No se pudo ejecutar el escáner USB\n");
            }
        }
        
        // Si no hay contenido, agregar mensaje por defecto
        if (strlen(temp) == 0) {
            snprintf(temp, sizeof(temp), "📁 Escáner USB: Sin datos disponibles\n");
        }

        // Actualizar reporte global
        pthread_mutex_lock(&report_mutex);
        snprintf(fs_report, sizeof(fs_report), "%s", temp);
        pthread_mutex_unlock(&report_mutex);

        // Guardar en archivo
        system("mkdir -p Report");
        FILE *out = fopen("Report/reporte_sistema_archivos.txt", "w");
        if (out) {
            fputs(temp, out);
            fclose(out);
        }
        
        sleep(10);
    }
    pthread_exit(NULL);
}

// Función para escanear memoria
void *scanMemory(void *arg) {
    const char *source_file = "Scanners/Process/process_monitor.c";
    const char *executable = "./Scanners/Process/proccess_monitor";
    
    while (keep_running) {
        char buffer[4096];
        char temp[MAX_REPORT] = "";
        
        // Crear directorio si no existe
        system("mkdir -p Scanners/Process");
        
        // Compilar si es necesario
        if (compile_if_needed(source_file, executable, "-lpthread") != 0) {
            snprintf(temp, sizeof(temp), "❌ Error: No se pudo compilar o ejecutar el monitor de procesos\n");
        } else {
            // Ejecutar el monitor
            FILE *scanner = popen(executable, "r");
            if (scanner) {
                while (fgets(buffer, sizeof(buffer), scanner) && strlen(temp) < MAX_REPORT - 100) {
                    strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
                }
                pclose(scanner);
            } else {
                snprintf(temp, sizeof(temp), "❌ Error: No se pudo ejecutar el monitor de procesos\n");
            }
        }
        
        // Si no hay contenido, usar comando alternativo
        if (strlen(temp) == 0) {
            FILE *scanner = popen("free -h && echo '---' && ps aux --sort=-%mem | head -10", "r");
            if (scanner) {
                while (fgets(buffer, sizeof(buffer), scanner) && strlen(temp) < MAX_REPORT - 100) {
                    strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
                }
                pclose(scanner);
            } else {
                snprintf(temp, sizeof(temp), "💾 Monitor de memoria: Sin datos disponibles\n");
            }
        }

        // Actualizar reporte global
        pthread_mutex_lock(&report_mutex);
        snprintf(mem_report, sizeof(mem_report), "%s", temp);
        pthread_mutex_unlock(&report_mutex);

        // Guardar en archivo
        FILE *out = fopen("Report/reporte_memoria.txt", "w");
        if (out) {
            fputs(temp, out);
            fclose(out);
        }
        
        sleep(10);
    }
    pthread_exit(NULL);
}

// Función para escanear puertos
void *scanPorts(void *arg) {
    const char *source_file = "Scanners/Ports/port_scanner.c";
    const char *executable = "./Scanners/Ports/port_scanner";
    
    while (keep_running) {
        char cmd[256];
        char ip[] = "127.0.0.1";
        int start_port = 1, end_port = 65535; // Reducido para evitar timeouts largos
        char buffer[4096];
        char temp[MAX_REPORT] = "";
        
        // Crear directorio si no existe
        system("mkdir -p Scanners/Ports");
        
        // Compilar si es necesario
        if (compile_if_needed(source_file, executable, "-lpthread") != 0) {
            snprintf(temp, sizeof(temp), "❌ Error: No se pudo compilar o ejecutar el escáner de puertos\n");
        } else {
            // Ejecutar el escáner
            snprintf(cmd, sizeof(cmd), "%s %s %d %d", executable, ip, start_port, end_port);
            FILE *scanner = popen(cmd, "r");
            if (scanner) {
                while (fgets(buffer, sizeof(buffer), scanner) && strlen(temp) < MAX_REPORT - 100) {
                    strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
                }
                pclose(scanner);
            } else {
                snprintf(temp, sizeof(temp), "❌ Error: No se pudo ejecutar el escáner de puertos\n");
            }
        }
        
        // Si no hay contenido, usar netstat como alternativa
        if (strlen(temp) == 0) {
            FILE *scanner = popen("netstat -tuln 2>/dev/null | head -20", "r");
            if (scanner) {
                while (fgets(buffer, sizeof(buffer), scanner) && strlen(temp) < MAX_REPORT - 100) {
                    strncat(temp, buffer, sizeof(temp) - strlen(temp) - 1);
                }
                pclose(scanner);
            } else {
                snprintf(temp, sizeof(temp), "🔌 Escáner de puertos: Sin datos disponibles\n");
            }
        }

        // Actualizar reporte global
        pthread_mutex_lock(&report_mutex);
        snprintf(port_report, sizeof(port_report), "%s", temp);
        pthread_mutex_unlock(&report_mutex);

        // Guardar en archivo
        FILE *out = fopen("Report/reporte_puertos.txt", "w");
        if (out) {
            fputs(temp, out);
            fclose(out);
        }
        
        sleep(10);
    }
    pthread_exit(NULL);
}

//Escanear puertos de forma interactiva
void scanPortsInteractive() {
    char ip[64] = "127.0.0.1";
    char port_range[32] = "";
    int start_port = 1, end_port = 65535;

    printf("Ingrese la IP a escanear (dejar vacío para localhost): ");
    if (fgets(ip, sizeof(ip), stdin) == NULL) {
        printf("❌ Error al leer la IP.\n");
        return;
    }
    ip[strcspn(ip, "\n")] = 0;
    if (strlen(ip) == 0) strcpy(ip, "127.0.0.1");

    printf("Ingrese el rango de puertos (ej: 1-1024, dejar vacío para 1-65535): ");
    if (fgets(port_range, sizeof(port_range), stdin) == NULL) {
        printf("❌ Error al leer el rango de puertos.\n");
        return;
    }
    port_range[strcspn(port_range, "\n")] = 0;
    if (strlen(port_range) == 0) {
        start_port = 1;
        end_port = 65535;
    } else if (sscanf(port_range, "%d-%d", &start_port, &end_port) != 2 ||
                start_port < 1 || end_port > 65535 || start_port > end_port) {
        printf("Rango de puertos inválido. Use: inicio-fin (ej: 1-1024)\n");
        // Limpiar buffer solo si hay basura
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return;
    }

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "./Scanners/Ports/port_scanner %s %d %d", ip, start_port, end_port);

    printf("\n\033[1;36m~~~~~~~~~~~~~~>>> Resultado del escaneo personalizado <<<~~~~~~~~~~~~~~\033[0m\n\n");
    FILE *scanner = popen(cmd, "r");
    if (scanner) {
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), scanner)) {
            fputs(buffer, stdout);
        }
        pclose(scanner);
    } else {
        printf("❌ Error al ejecutar el escáner de puertos.\n");
    }
    printf("\n\033[1;33m------------------------------------------------------------------------\033[0m\n");
}

// Funciones para imprimir los reportes en consola
void print_fs_report() {
    pthread_mutex_lock(&report_mutex);
    printf("\n\033[1;32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ REPORTE USB ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");
    printf("\n%s\n", fs_report);
    pthread_mutex_unlock(&report_mutex);
    printf("\033[1;33m------------------------------------------------------------------------\033[0m\n");
}

void print_mem_report() {
    pthread_mutex_lock(&report_mutex);
    printf("\n\033[1;32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ REPORTE MEMORIA ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");
    printf("\n%s\n", mem_report);
    pthread_mutex_unlock(&report_mutex);
    printf("\033[1;33m------------------------------------------------------------------------\033[0m\n\n");
}

void print_port_report() {
    pthread_mutex_lock(&report_mutex);
    printf("\n\033[1;32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ REPORTE PUERTOS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");
    printf("\n%s\n", port_report);
    pthread_mutex_unlock(&report_mutex);
    printf("\033[1;33m------------------------------------------------------------------------\033[0m\n");
}

void GenerateUnifiedReport() {
    // Verificar que los archivos de reporte existan
    const char *files[] = {
        "Report/reporte_sistema_archivos.txt",
        "Report/reporte_memoria.txt", 
        "Report/reporte_puertos.txt"
    };
    
    int all_exist = 1;
    for (int i = 0; i < 3; i++) {
        if (!file_exists(files[i])) {
            all_exist = 0;
            break;
        }
    }
    
    if (!all_exist) {
        printf("⏳ Generando reportes antes de crear el archivo unificado...\n");
        sleep(5); // Dar tiempo a que se generen los reportes
    }

    // Crear archivo unificado
    FILE *unified = fopen("Report/reporte_completo.txt", "w");
    if (!unified) {
        printf("❌ Error: No se pudo crear el archivo unificado\n");
        return;
    }

    // Escribir encabezado
    fprintf(unified, "===============================================================================\n");
    fprintf(unified, "                    REPORTE COMPLETO DEL SISTEMA\n");
    fprintf(unified, "                    Generado: %s", __DATE__);
    fprintf(unified, "\n===============================================================================\n\n");

    // Agregar cada reporte
    const char *titles[] = {
        "REPORTE DE SISTEMA DE ARCHIVOS (USB)",
        "REPORTE DE MEMORIA Y PROCESOS", 
        "REPORTE DE PUERTOS"
    };

    for (int i = 0; i < 3; i++) {
        fprintf(unified, "\n>>> %s <<<\n", titles[i]);
        fprintf(unified, "-------------------------------------------------------------------------------\n");
        
        FILE *report_file = fopen(files[i], "r");
        if (report_file) {
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), report_file)) {
                fputs(buffer, unified);
            }
            fclose(report_file);
        } else {
            fprintf(unified, "❌ Error: No se pudo leer %s\n", files[i]);
        }
        
        fprintf(unified, "\n===============================================================================\n");
    }

    fclose(unified);
    
    printf("\n\033[1;32m~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ REPORTE UNIFICADO GENERADO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\033[0m\n");
    printf("📄 Reporte completo guardado en: Report/reporte_completo.txt\n");
    printf("📊 Puedes abrirlo con cualquier editor de texto\n");
}

void abrir_reporte_txt(const char *nombre_archivo) {
    char ruta_archivo[256];
    snprintf(ruta_archivo, sizeof(ruta_archivo), "Report/%s", nombre_archivo);

    if (!file_exists(ruta_archivo)) {
        printf("❌ El archivo no existe. Genérelo primero.\n");
        return;
    }

    // Intentar abrir con diferentes editores disponibles
    if (system("which gedit >/dev/null 2>&1") == 0) {
        char comando[512];
        snprintf(comando, sizeof(comando), "gedit \"%s\" &", ruta_archivo);
        system(comando);
        printf("📝 Abriendo archivo con gedit...\n");
    } else if (system("which nano >/dev/null 2>&1") == 0) {
        char comando[512];
        snprintf(comando, sizeof(comando), "nano \"%s\"", ruta_archivo);
        printf("📝 Abriendo archivo con nano (presiona Ctrl+X para salir)...\n");
        system(comando);
    } else if (system("which vim >/dev/null 2>&1") == 0) {
        char comando[512];
        snprintf(comando, sizeof(comando), "vim \"%s\"", ruta_archivo);
        printf("📝 Abriendo archivo con vim (presiona :q para salir)...\n");
        system(comando);
    } else if (system("which cat >/dev/null 2>&1") == 0) {
        char comando[512];
        snprintf(comando, sizeof(comando), "cat \"%s\"", ruta_archivo);
        printf("📝 Mostrando contenido del archivo:\n");
        printf("===============================================================================\n");
        system(comando);
        printf("\n===============================================================================\n");
    } else {
        printf("❌ No se encontró ningún editor de texto disponible.\n");
        printf("Instale uno con: sudo apt install gedit nano vim\n");
    }
}

int main() {
    int option ;
    pthread_t fs_tid, mem_tid, port_tid;

    printf("🛡️  Bienvenido al Gran Salón del Trono de Matcom Guard\n");
    printf("🚀 Iniciando Sistema de Monitoreo...\n");
    
    // Inicializar mutex
    pthread_mutex_init(&report_mutex, NULL);

    // Crear directorio para reportes si no existe
    system("mkdir -p Report Scanners/USB Scanners/Process Scanners/Ports");

    inicializar_alertas_guard();
    inicializar_mutex_alertas();
    printf("✅ Sistema de alertas inicializado\n");
    
    printf("⚡ Lanzando escáneres en segundo plano...\n");
    
    // Lanzar los escaneos en hilos
    if (pthread_create(&fs_tid, NULL, scanFilesystem, NULL) != 0) {
        printf("❌ Error al crear hilo de escaneo de archivos\n");
    }
    
    if (pthread_create(&mem_tid, NULL, scanMemory, NULL) != 0) {
        printf("❌ Error al crear hilo de escaneo de memoria\n");
    }
    
    if (pthread_create(&port_tid, NULL, scanPorts, NULL) != 0) {
        printf("❌ Error al crear hilo de escaneo de puertos\n");
    }
    
    // Dar tiempo para que se inicialicen los reportes
    printf("⏳ Esperando inicialización de escáneres...\n");
    sleep(7);

    // Abrir terminal para mostrar alertas en tiempo real
    system("gnome-terminal -- bash -c 'tail -n +1 -f Report/alertas_guard.txt' &");
    //system("xterm -hold -e 'tail -n +1 -f Report/alertas_guard.txt' &");
    
    printf("🔔 Terminal de alertas abierta. Monitoreando eventos...\n");

    sleep(3); // Esperar a que se abra la terminal de alertas

    // Ciclo principal del menú
    while (1) {

        //system("clear");
        
        // Mostrar menú de usuario
        printf("\033[1;33m========================================\033[0m\n");
        printf("\n\033[1;36m=== 🛡️ Gran Salón del Trono 🛡️ ===\033[0m\n");
        printf("\033[1;33m1.\033[0m \033[0;32mMostrar reporte de sistema de archivos\033[0m\n");
        printf("\033[1;33m2.\033[0m \033[0;32mMostrar reporte de memoria\033[0m\n");
        printf("\033[1;33m3.\033[0m \033[0;32mMostrar reporte de puertos\033[0m\n");
        printf("\033[1;33m4.\033[0m \033[0;32mMostrar todos los reportes\033[0m\n");
        printf("\033[1;33m5.\033[0m \033[0;34mGenerar reporte unificado (TXT)\033[0m\n");
        printf("\033[1;33m6.\033[0m \033[0;34mAbrir reporte completo\033[0m\n");
        printf("\033[1;33m7.\033[0m \033[1;31mSalir\033[0m\n");
        printf("\n");
        printf("\033[1;33m========================================\033[0m\n");
        printf("\033[1;35mSelecciona una opción:\033[0m ");

        
        if (scanf("%d", &option) != 1) {
            printf("❌ Entrada inválida. Inténtalo nuevamente.\n");
            while (getchar() != '\n'); // Limpiar buffer
            sleep(2);
            continue;
        }
        getchar(); // Limpiar salto de línea pendiente
        
        /*
        char input[32];
        if (!fgets(input, sizeof(input), stdin)) {
            printf("❌ Entrada inválida. Inténtalo nuevamente.\n");
            sleep(2);
            continue;
        }
        option = atoi(input);
        */

        switch (option) {
            case 1:
                print_fs_report();
                break;
            case 2:
                print_mem_report();
                break;
            case 3:
                scanPortsInteractive();
                break;
            case 4:
                print_fs_report();
                print_mem_report();
                print_port_report();
                printf("\n✅ Todos los reportes mostrados\n");
                break;
            case 5:
                GenerateUnifiedReport();
                break;
            case 6:
                abrir_reporte_txt("reporte_completo.txt");
                break;
            case 7:
                printf("🔄 Cerrando hilos...\n");
                keep_running = 0;
                
                // Esperar a que terminen los hilos (con timeout)
                struct timespec timeout = {2, 0}; // 2 segundos
                pthread_timedjoin_np(fs_tid, NULL, &timeout);
                pthread_timedjoin_np(mem_tid, NULL, &timeout);
                pthread_timedjoin_np(port_tid, NULL, &timeout);
                
                pthread_mutex_destroy(&report_mutex);
                destruir_alertas_guard();
                destruir_mutex_alertas();
                
                printf("✅ Hilos cerrados correctamente.\n");
                printf("👋 Saliendo del programa...\n");
                return 0;
            default:
                printf("❌ Opción inválida. Inténtalo nuevamente.\n");
        }
        
        printf("\n⏸️  Presione Enter para continuar...");
        getchar();
        
        //system("clear"); // Limpiar pantalla después de cada acción
    }
    
    return 0;
}