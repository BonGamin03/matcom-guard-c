#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Función para escanear sistema de archivos
void scanFilesystem() {
    printf("\n=== Escaneando sistema de archivos 🔄 ===\n");
    system("ls -lah > Report/reporte_sistema_archivos.txt");
    printf("🔍 Escaneo de sistema de archivos completado. 📄 Reporte guardado en 'Report/reporte_sistema_archivos.txt'.\n");
}

// Función para escanear memoria (usando el comando 'free' en Linux)
void scanMemory() {
    printf("\n=== Escaneando memoria 🔄 ===\n");
    system("free -h > Report/reporte_memoria.txt");
    printf("🔍 Escaneo de memoria completado. 📄 Reporte guardado en 'Report/reporte_memoria.txt'.\n");
}

// Función para escanear puertos abiertos (usando 'netstat' o 'ss')
void scanPorts() {
    printf("\n=== Escaneando puertos 🔄 ===\n");
    system("./Scanners/port_scanner > Report/reporte_puertos.txt");
    printf("🔍 Escaneo de puertos completado. 📄 Reporte guardado en 'Report/reporte_puertos.txt'.\n");
}
/*
// Función para escanear puertos
void escanear_puertos() {
    printf("\n=== Escaneando puertos ===\n");
    system("netstat -tuln");  // Puertos abiertos
    system("ss -tuln");  // Alternativa moderna
}
*/

// Función para escanear todo
void scanAll() {
    scanFilesystem();
    scanMemory();
    scanPorts();
}

// Función para exportar reporte a PDF (requiere 'enscript' y 'ps2pdf')
void exportToPDF() {
    system("enscript -p Report/reporte.ps Report/reporte_sistema_archivos.txt Report/reporte_memoria.txt Report/reporte_puertos.txt");
    system("ps2pdf Report/reporte.ps Report/reporte.pdf");
    printf("Reporte exportado a PDF en 'Report/reporte.pdf'.\n");
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
/*
void abrir_pdf_linux(const char *nombre_pdf) {
    if (access(nombre_pdf, F_OK) == -1) {
        printf("El PDF no existe. Genérelo primero.\n");
        return;
    }

    if (system("which xdg-open >/dev/null") == 0) {
        char comando[256];
        snprintf(comando, sizeof(comando), "xdg-open \"%s\" &", nombre_pdf);
        system(comando);
    } else {
        printf("Instale xdg-open para visualizar PDFs:\n");
        printf("sudo apt install xdg-utils\n");
    }
}
*/


int main() {
    int option;

    while (1) {
        printf("\n=== 🛡️ Gran Salón del Trono 🛡️ ===\n");
        printf("1. Escanear sistema de archivos\n");
        printf("2. Escanear memoria\n");
        printf("3. Escanear puertos\n");
        printf("4. Escanear todo\n");
        printf("5. Exportar reporte a PDF\n");
        printf("6. Abrir PDF\n");
        printf("7. Salir\n");
        printf("Selecciona una opción: ");
        scanf("%d", &option);

        switch (option) {
            case 1:
                scanFilesystem();
                break;
            case 2:
                scanMemory();
                break;
            case 3:
                scanPorts();
                break;
            case 4:
                scanAll();
                break;
            case 5:
                exportToPDF();
                break;
            case 6:
                abrir_pdf_linux("reporte.pdf");
                break;
            case 7:
                printf("Saliendo del programa...\n");
                return 0;
            default:
                printf("Opción inválida. Inténtalo nuevamente.\n");
        }
        printf("\npresione enter para continuar...");
        getchar();
        getchar();
        system("clear");
        
    }

    return 0;
}