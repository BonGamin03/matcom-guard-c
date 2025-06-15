
/*
Características del programa:

    Escanea un rango de puertos TCP especificado

    Identifica servicios comunes asociados a los puertos abiertos

    Detecta puertos potencialmente comprometidos basándose en:

        Puertos comúnmente usados para backdoors (31337, 6667, etc.)

        Puertos altos (>1024) sin servicios conocidos asociados

    Genera un informe con:

        Listado de puertos abiertos

        Clasificación de puertos (normales vs sospechosos)

        Resumen estadístico del escaneo

El programa utiliza conexiones TCP activas para determinar el estado de los puertos, con un timeout configurable para evitar bloqueos prolongados.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

// Estructura para mapear puertos a servicios conocidos
typedef struct
{
    int port;
    const char *service;
} PortService;

// Lista de servicios comunes
PortService common_services[] = {
    {20, "FTP (Data)"},
    {21, "FTP (Control)"},
    {22, "SSH"},
    {23, "Telnet"},
    {25, "SMTP"},
    {53, "DNS"},
    {80, "HTTP"},
    {110, "POP3"},
    {143, "IMAP"},
    {161, "SNMP"},
    {389, "LDAP"},
    {443, "HTTPS"},
    {445, "Microsoft-DS"},
    {465, "SMTPS"},
    {514, "Syslog"},
    {587, "Submission (SMTP)"},
    {993, "IMAPS"},
    {995, "POP3S"},
    {1433, "Microsoft SQL Server"},
    {1521, "Oracle DB"},
    {1723, "PPTP"},
    {3306, "MySQL"},
    {3389, "RDP"},
    {4444, "Sub 7 Backdoor"},
    {5000, "UPnP"},
    {5001, "Rsync"},
    {5002, "Docker (API)"},
    {5003, "Docker (Swarm)"},
    {5004, "Docker (Swarm)"},
    {5005, "Docker (Swarm)"},
    {5432, "PostgreSQL"},
    {5900, "VNC"},
    {6000, "X11"},
    {6379, "Redis"},
    {6666, "Backdoor común"},
    {6667, "IRC"},
    {6969, "Backdoor común"},
    {8000, "HTTP alternativo 2"},
    {8069, "Odoo"},
    {8080, "HTTP alternativo"},
    {8081, "HTTP alternativo 3"},
    {9000, "HTTP alternativo 4"},
    {9200, "Elasticsearch"},
    {9300, "Elasticsearch (Transport)"},
    {10000, "Webmin"},
    {11211, "Memcached"},
    {12345, "NetBus Backdoor"},
    {27017, "MongoDB"},
    {27018, "MongoDB (Secundario)"},
    {31337, "Back Orifice"},
    {54321, "NetBus Pro Backdoor"},
    {0, NULL} // Fin de la lista
};

// Función para obtener el nombre del servicio asociado a un puerto
const char *get_service_name(int port)
{
    for (int i = 0; common_services[i].service != NULL; i++)
    {
        if (common_services[i].port == port)
        {
            return common_services[i].service;
        }
    }
    return "Desconocido";
}

// Función para verificar si un puerto es sospechoso
int is_suspicious_port(int port)
{
    // Lista de puertos comúnmente usados para backdoors o sospechosos
    //int suspicious_ports[] = {31337, 6667, 4444, 12345, 54321, 6000, 6666, 6969, 0};
    // Lista extendida de puertos comúnmente usados para backdoors o sospechosos
    // Fuente: https://www.speedguide.net/ports.php y otras referencias de seguridad
    int suspicious_ports[] = {
        31337, // Back Orifice
        6667,  // IRC
        4444,  // Sub7, Metasploit
        12345, // NetBus
        54321, // NetBus Pro
        6000,  // X11
        6666,  // Backdoor común
        6969,  // Backdoor común
        27444, // Trinoo
        27665, // Trinoo
        20034, // NetBus 2 Pro
        31335, // Back Orifice
        31338, // Deep Throat
        31339, // NetSpy
        31340, // The Invasor
        31341, // BLA trojan
        31343, // Back Orifice
        31344, // Back Orifice
        31345, // Back Orifice
        31346, // Back Orifice
        31348, // Back Orifice
        31350, // Back Orifice
        54320, // Backdoor
        16959, // ZeroAccess
        65000, // Devil
        65001, // Devil
        65002, // Devil
        65003, // Devil
        65004, // Devil
        65005, // Devil
        65006, // Devil
        65007, // Devil
        65008, // Devil
        65009, // Devil
        0
    };

    for (int i = 0; suspicious_ports[i] != 0; i++)
    {
        if (port == suspicious_ports[i])
        {
            return 1;
        }
    }

    // Considerar sospechosos puertos altos sin servicio conocido
    //if (port > 1024 && get_service_name(port) == "Desconocido")
    if (port > 1024 && strcmp(get_service_name(port), "Desconocido") == 0)
    {
        return 1;
    }
    return 0;
}

// Función para escanear un puerto
int scan_port(const char *ip, int port, int timeout_sec)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Error al crear socket");
        return -1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server.sin_addr);

    // Establecer timeout
    struct timeval timeout;
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("Error al establecer timeout");
        close(sock);
        return -1;
    }

    // Intentar conectar
    int result = connect(sock, (struct sockaddr *)&server, sizeof(server));
    close(sock);

    return (result == 0) ? 1 : 0;
}

int main(int argc, char *argv[])
{
    // Verificar argumentos
    if (argc != 4)
    {
        printf("Uso: %s <IP> <Puerto_Inicial> <Puerto_Final>\n", argv[0]);
        printf("Ejemplo: %s 127.0.0.1 1-1024\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int start_port= atoi(argv[2]);
    int  end_port= atoi(argv[3]);


    //Modo Interactivo
/*

    char ip[32];
    int start_port, end_port;
    printf("Ingrese la IP a escanear (dejar vacío para localhost): ");
    //  fflush(stdout);
    // Leer línea completa para detectar si está vacía
    char input[64];
    if (fgets(input, sizeof(input), stdin) != NULL) {
        // Si solo se presionó Enter, usar localhost
        if (input[0] == '\n') {
            strcpy(ip, "127.0.0.1");
        } else {
            // Eliminar salto de línea si existe
            input[strcspn(input, "\n")] = 0;
            strncpy(ip, input, sizeof(ip) - 1);
            ip[sizeof(ip) - 1] = '\0';
        }
    } else {
        // En caso de error, usar localhost
        strcpy(ip, "127.0.0.1");
    }

    printf("Ingrese el rango de puertos (ej: 1-1024):");
    fflush(stdout); 
    char port_range[64];
    if (fgets(port_range, sizeof(port_range), stdin) != NULL) {
        port_range[strcspn(port_range, "\n")] = 0;
    }

    if (sscanf(port_range, "%d-%d", &start_port, &end_port) != 2) {
        printf("Formato de rango de puertos inválido. Use: inicio-fin (ej: 1-1024)\n");
        return 1;
    }
    */


    printf("IP a escanear: %s\n", ip);
    printf("Puerto Inicial: %d\n", start_port);
    printf("Puerto Final: %d\n", end_port);

    printf("\nEscaneando puertos TCP en %s del rango %d-%d...\n\n", ip, start_port, end_port);

    int open_ports = 0;
    int suspicious_ports = 0;

    // Escanear cada puerto en el rango
    for (int port = start_port; port <= end_port; port++)
    {
        if (scan_port(ip, port, 1))
        { // Timeout de 1 segundo
            const char *service = get_service_name(port);
            int suspicious = is_suspicious_port(port);

            if (suspicious)
            {
                printf("🚨 [ALERTA] Puerto %d/tcp abierto (%s) - posible backdoor o servicio sospechoso.\n",
                       port, service);
                suspicious_ports++;
            }
            else
            {
                printf("✅ [OK] Puerto %d/tcp (%s) abierto (esperado).\n", port, service);
            }

            open_ports++;
        }
    }

    printf("\nResumen del escaneo:\n");
    printf(" - Puertos escaneados: %d\n", end_port - start_port + 1);
    printf(" - Puertos abiertos: %d\n", open_ports);
    printf(" - Puertos sospechosos: %d\n", suspicious_ports);

    return 0;
}


