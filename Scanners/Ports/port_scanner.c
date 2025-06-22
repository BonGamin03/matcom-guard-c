
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
#include "../../Scanners/util.h"


// Estructura para mapear puertos a servicios conocidos
typedef struct
{
    int port;
    const char *service;
    const char *expected_process; // Nuevo: proceso esperado
} PortService;

// Lista de servicios comunes y procesos esperados
PortService common_services[] = {
    {20, "FTP (Data)", "vsftpd"},
    {21, "FTP (Control)", "vsftpd"},
    {22, "SSH", "sshd"},
    {23, "Telnet", "telnetd"},
    {25, "SMTP", "master"},
    {53, "DNS", "named"},
    {80, "HTTP", "apache2"},
    {110, "POP3", "dovecot"},
    {143, "IMAP", "dovecot"},
    {161, "SNMP", "snmpd"},
    {389, "LDAP", "slapd"},
    {443, "HTTPS", "apache2"},
    {445, "Microsoft-DS", "smbd"},
    {465, "SMTPS", "master"},
    {514, "Syslog", "rsyslogd"},
    {587, "Submission (SMTP)", "master"},
    {993, "IMAPS", "dovecot"},
    {995, "POP3S", "dovecot"},
    {1433, "Microsoft SQL Server", "sqlservr"},
    {1521, "Oracle DB", "oracle"},
    {1723, "PPTP", "pptpd"},
    {3306, "MySQL", "mysqld"},
    {3389, "RDP", "Xvnc"},
    {4444, "Sub 7 Backdoor", NULL},
    {5000, "UPnP", "minissdpd"},
    {5432, "PostgreSQL", "postgres"},
    {5900, "VNC", "Xvnc"},
    {6000, "X11", "Xorg"},
    {6379, "Redis", "redis-server"},
    {6666, "Backdoor común", NULL},
    {6667, "IRC", "ircd"},
    {6969, "Backdoor común", NULL},
    {8000, "HTTP alternativo 2", "apache2"},
    {8069, "Odoo", "odoo"},
    {8080, "HTTP alternativo", "apache2"},
    {8081, "HTTP alternativo 3", "apache2"},
    {9000, "HTTP alternativo 4", "apache2"},
    {9200, "Elasticsearch", "java"},
    {9300, "Elasticsearch (Transport)", "java"},
    {10000, "Webmin", "miniserv.pl"},
    {11211, "Memcached", "memcached"},
    {12345, "NetBus Backdoor", NULL},
    {27017, "MongoDB", "mongod"},
    {27018, "MongoDB (Secundario)", "mongod"},
    {31337, "Back Orifice", NULL},
    {54321, "NetBus Pro Backdoor", NULL},
    {0, NULL, NULL} // Fin de la lista
};

// Función para obtener el nombre del servicio y proceso esperado asociado a un puerto
const PortService *get_service_info(int port)
{
    for (int i = 0; common_services[i].service != NULL; i++)
    {
        if (common_services[i].port == port)
        {
            return &common_services[i];
        }
    }
    return NULL;
}

// Función para verificar si un puerto es sospechoso (criterio avanzado)
int is_suspicious_port(int port)
{
    int suspicious_ports[] = {
        31337, 6667, 4444, 12345, 54321, 6000, 6666, 6969, 27444, 27665, 20034,
        31335, 31338, 31339, 31340, 31341, 31343, 31344, 31345, 31346, 31348, 31350,
        54320, 16959, 65000, 65001, 65002, 65003, 65004, 65005, 65006, 65007, 65008, 65009, 0
    };
    for (int i = 0; suspicious_ports[i] != 0; i++)
    {
        if (port == suspicious_ports[i])
        {
            return 1;
        }
    }
    // Considerar sospechosos puertos altos sin servicio conocido
    const PortService *info = get_service_info(port);
    if (port > 1024 && (!info || strcmp(info->service, "Desconocido") == 0))
    {
        return 1;
    }
    return 0;
}

// Función para obtener el proceso que escucha en un puerto (requiere sudo)
void get_process_for_port(int port, char *result, size_t size)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sudo ss -ltnp | grep ':%d ' | awk '{print $NF}'", port);
    FILE *fp = popen(cmd, "r");
    if (fp)
    {
        if (fgets(result, size, fp) == NULL)
            snprintf(result, size, "Desconocido");
        else
        {
            // Limpiar salto de línea
            size_t len = strlen(result);
            if (len > 0 && result[len - 1] == '\n')
                result[len - 1] = '\0';
        }
        pclose(fp);
    }
    else
    {
        snprintf(result, size, "Desconocido");
    }
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
    const char *ip = argv[1];
    int start_port = atoi(argv[2]);
    int end_port = atoi(argv[3]);

    printf("\n");
    printf("IP a escanear: %s\n", ip);
    printf("Puerto Inicial: %d\n", start_port);
    printf("Puerto Final: %d\n", end_port);

    printf("\nEscaneando puertos TCP en %s del rango %d-%d...\n\n", ip, start_port, end_port);

    int open_ports = 0;
    int suspicious_ports = 0;

    for (int port = start_port; port <= end_port; port++)
    {
        if (scan_port(ip, port, 1))
        {
            const PortService *info = get_service_info(port);
            int suspicious = is_suspicious_port(port);

            char process[128] = "Desconocido";
            get_process_for_port(port, process, sizeof(process));

            int process_match = 1;
            if (info && info->expected_process && strstr(process, info->expected_process) == NULL)
                process_match = 0;

            if (suspicious || !process_match)
            {
                printf("🚨 [ALERTA] Puerto %d/tcp abierto (%s) - Proceso: %s - ", port, info ? info->service : "Desconocido", process);
                if (suspicious)
                    printf("posible backdoor o servicio sospechoso");
                if (!process_match)
                    printf("%s%sproceso inesperado para este puerto", suspicious ? " y " : "", suspicious ? "" : "");
                printf(".\n");
                suspicious_ports++;

                // Escribir alerta en el archivo
                char alert_message[512];
                snprintf(alert_message, sizeof(alert_message), "🚨 ALERTA PUERTOS:  Puerto %d/tcp abierto por %s(proceso: %s) - posible backdoor o servicio sospechoso.", port, info ? info->service : "Desconocido", process);
                Write_Alert(alert_message);
                //system("cat Report/alertas_guard.txt"); //Muestra las alertas en tiempo real
            }
            else
            {
                printf("✅ [OK] Puerto %d/tcp (%s) abierto por %s (esperado).\n", port, info ? info->service : "Desconocido", process);
            }
            open_ports++;
        }
    }

    printf("\nResumen del escaneo:\n");
    printf(" - Puertos escaneados: %d\n", end_port - start_port + 1);
    printf(" - Puertos abiertos: %d\n", open_ports);
    printf(" - Puertos sospechosos: %d\n", suspicious_ports);
    printf(" - Puertos cerrados: %d\n", (end_port - start_port + 1) - open_ports);
    printf("\n");
    if (suspicious_ports > 0)
    {
        printf("⚠️ Se detectaron puertos. Se recomienda investigar más a fondo.\n");
    }
    else
    {
        printf("✅ No se detectaron puertos. El sistema parece estar limpio.\n");
    }
    printf("\n");

    return 0;
}