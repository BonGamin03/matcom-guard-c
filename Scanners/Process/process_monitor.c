#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include "process_monitor.h"
#include "../../Scanners/util.h"
#include <pthread.h>

//Constantes para limites del sistema
static float umbral_RAM = 50.0;
static float umbral_CPU = 70.0;
static int monitor_Time = 5;

//Variables globales
static Process processes[Max_Processes];    //Array de todos los procesos
static int num_processes = 0;               //Contador de procesos activos
static unsigned long last_total_CPU = 0;    //Tiempo total de CPU del sistema(lectura anterior) 

//Funcion para cargar la configuracion inicial
void load_config(){
    FILE* config_file;
    char line[256];
    char key[128] , value[128];

    //Intentar abrir el archivo de configuracion
    config_file = fopen("../etc/matcomguard.conf","r");
    if(config_file == NULL){
        printf(" [INFO] No se pudo abrir el archivo de configuracion , se usaran los valores por defecto");
        return;
    }

    printf(" [INFO] Cargando configuracion...\n");

    //Leer linea por linea del archivo de configuracion
    while(fgets(line , sizeof(line) , config_file)){
        //Ignorar comentarios y saltos de linea
        if(line[0] == '#' || line[0] == '\n'){
            continue;
        }

        if(sscanf(line,"%[^=]=%s",key,value) == 2){
            if(strcmp(key,"Umbral_CPU") == 0){
                umbral_CPU = atof(value);
                printf("[CONFIG] Umbral CPU : %.1f%%\n",umbral_CPU);
            }
            else if(strcmp(key,"Umbral_RAM") == 0){
                umbral_RAM = atof(value);
                printf("[CONFIG] Umbral RAM : %.1f%%\n",umbral_RAM);
            }
            else if(strcmp(key,"Monitor_Time") == 0){
                monitor_Time = atoi(value);
                printf("[CONFIG] Duracion alerta : %d segundos\n",monitor_Time);
            }
        }
    }

    fclose(config_file);
}

//Lista de procesos que no deben generar alertas
static const char* white_list[] = {
    "gcc", "g++" , "make",             //Compiladores
    "gnome-shell", "Xorg",             //Procesos de interfaz grafica
    "systemd",                         //Gestor de servicos del sistema
    "kthreadd", "ksoftirqd",           //Hilos del kernel
    "migration", "rcu_", "watchdog",   //Procesos internos del kernel
    "kernel", "init",                  //Procesos fundamentales del sistema
    "dbus","NetworkManager",           //Servicios de sistema
    "pulseaudio",                      //Servicio de audio
    NULL                               //Terminador de lista
};

void print_module(void) {
    // Imprime un banner decorativo usando caracteres Unicode
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                    MATCOM GUARD - MÓDULO 2                    ║\n");
    printf("║                GUARDIAS DEL TESORO REAL                       ║\n");
    printf("║                   Monitor de Procesos                         ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    // Muestra la configuración actual de umbrales
    printf("🛡️  Umbrales: CPU > %.1f%%, RAM > %.1f%%\n", umbral_CPU, umbral_RAM);
    // Muestra el intervalo de monitoreo
    printf("⏱️  Tiempo de monitoreo: %d segundos entre lecturas\n", monitor_Time);
    // Instrucciones para el usuario
    printf("🔍 Presiona Ctrl+C para detener...\n\n");
}

//Inicializa el array con valores por defecto
void init_process(void){
    for(int i = 0;i<Max_Processes;i++){
        processes[i].pid = 0;
        processes[i].active = 0;
        processes[i].last_total_time = 0;
        processes[i].cpu_usage = 0.0;
        processes[i].mem_usage = 0.0;
        memset(processes[i].name,0,256);   //Limpia el nombre del proceso
    }
    num_processes = 0;
    last_total_CPU = 0;
}

//Verifica si un proceso esta en la lista blanca
int is_whitelist_process(const char* name){
    if(!name) return 0;
    for(int i = 0;white_list[i] != NULL;i++){
        if(strstr(name,white_list[i]) != NULL){
            return 1;
        }
    }
    return 0;
}

//Obtiene el tiempo total de CPU
unsigned long get_total_CPU(void){
    FILE* file = fopen("/proc/stat","r");
    if(!file){
        perror("Error leyendo la direccion proc/stat");
        return 0;
    }
    //Variables para almacenar los diferentes tiempos de CPU
    unsigned long user, nice, system, idle, iowait , irq, softirq, steal;
    int result = fscanf(file,"cpu %lu %lu %lu %lu %lu %lu %lu %lu",&user,&nice,&system,&idle,&iowait,&irq,&softirq,&steal);
    fclose(file);
    if(result!=8){
        printf("⚠️  Error parseando /proc/stat\n");
        return 0;
    }
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

//Obtiene la cantidad total de CPU
unsigned long get_total_memory(void){
    FILE* file = fopen("/proc/meminfo","r");
    if(!file){
        perror("Error leyendo la direccion /proc/meminfo");
        return 0;
    }
    char line[256];
    unsigned long total_mem = 0;
    while(fgets(line,sizeof(line),file)){
        if(strncmp(line,"MemTotal:",9) == 0){
            sscanf(line,"MemTotal : %lu kb",&total_mem);
            break;
        }
    }
    fclose(file);
    return total_mem;
}

//Lee la informacion de un proceso
int read_process(int pid,Process* process){
    if(!process) return 0;
    char path[256];
    FILE* file;
    snprintf(path,sizeof(path),"/proc/%d/stat",pid);
    file = fopen(path,"r");
    if(!file){
        return 0;
    }

    //Variables para leer datos del archivo stat
    unsigned long user_time, system_time , v_size ,rss; //Tiempos de CPU y memoria
    char temp_name[256];                                //Nombre temporal del proceso
    char state;                                          //Estado del proceso

    int result = fscanf(file,"%d %s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %lu %lu",
    &process->pid,temp_name,&state,&user_time,&system_time,&v_size,&rss);
    fclose(file);
    if(result < 7){
        return 0;
    }

    //Procesa el nombre del proceso
    size_t len = strlen(temp_name);
    if(len > 2 && temp_name[0] == '(' && temp_name[len-1] == ')'){
        strncpy(process->name,temp_name + 1,len - 2);
        process->name[len - 2] = '\0';
    }
    else{
        strncpy(process->name,temp_name,255);
        process->name[255] = '\0';
    }

    //Calcula el uso de CPU comparando con la lectura anterior
    unsigned long total_time = user_time + system_time;
    unsigned long CPU_total_actual = get_total_CPU();

    if(process->last_total_time > 0 && last_total_CPU > 0){
        unsigned long process_diff = total_time - process->last_total_time;
        unsigned long CPU_diff = CPU_total_actual - last_total_CPU;
        if(CPU_diff > 0){
            int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
            process->cpu_usage = (100.0 * process_diff / CPU_diff) * num_cores;
        }
        else{
            process->cpu_usage = 0.0;
        }
    }
    else{
        process->cpu_usage = 0.0;
    }

    //Actualiza valores para proxima comparacion
    process->last_user_time  = user_time;
    process->last_system_time = system_time;
    process->last_total_time = total_time;

    //Calculo uso de memoria
    unsigned long mem_total = get_total_memory();
    if(mem_total > 0){
        process->mem_usage = (100.0 * rss * 4) / mem_total;
    }
    else{
        process->mem_usage = 0.0;
    }
    process->active = 1;
    return 1;
}

//Escanea todos los procesos del sistema leyendo el directorio /proc
void scan_process(){
    DIR* proc_dir = opendir("/proc");
    if(!proc_dir){
        perror("Error abriendo /proc");
        return;
    }
    struct dirent* entry;   //Estructura para cada entrada del directorio

    //Marca todos los procesos como inactivos al inicio
    for(int i = 0;i<Max_Processes;i++){
        processes[i].active = 0;
    }

    //Lee cada entrada del directorio /proc
    while((entry = readdir(proc_dir)) != NULL){
        int pid = atoi(entry->d_name);
        if(pid > 0){
            int founded = -1;
            for(int i = 0;i < Max_Processes;i++){
                if(processes[i].pid == pid){
                    founded = i;
                    break;
                }
            }
            
            //Si es un proceso nuevo busca un slot libre
            if(founded == -1){
                for(int i = 0;i<Max_Processes;i++){
                    if(processes[i].pid == 0){
                        founded = i;
                        processes[i].pid = pid;
                        processes[i].last_total_time = 0;
                        break;
                    }
                }
            }

            //Si sen encontro slot lee la informacion
            if(founded != -1){
                if(read_process(pid,&processes[founded])){
                    if(founded >= num_processes){
                        num_processes = founded + 1;
                    }
                }
            }
        }
    }
    closedir(proc_dir);
    last_total_CPU = get_total_CPU();
}

//Verifica si algun proceso supera los umbrales y genera alertas
void check_alert(){
    printf("\n🔍 === VERIFICANDO ALERTAS ===\n");
    int alerts_founded = 0;

    //Recorre todos los procesos activos
    for(int i = 0;i<num_processes;i++){
        if(!processes[i].active || processes[i].pid == 0) continue;

        int generate_alert = 0; //Para determinar si hay alerta
        char text[256] = "";    //Para describir el motivo de la alerta

        //Verifica si supera el umbral de CPU
        if(processes[i].cpu_usage > umbral_CPU){
            if(!is_whitelist_process(processes[i].name)){
                generate_alert = 1;
                snprintf(text,sizeof(text),"CPU %1.f%% (umbral : %.1f%%)",processes[i].cpu_usage,umbral_CPU);
            }
        }

        //Verifica si supera el umbral de memoria
        if(processes[i].mem_usage > umbral_RAM){
            if(!is_whitelist_process(processes[i].name)){
                if(generate_alert){
                    strncat(text, "y",sizeof(text) - strlen(text) - 1);
                }
                else{
                    generate_alert = 1;
                }
                char temp[128];
                snprintf(temp,sizeof(temp),"RAM %.1f%% (umbral : %1f%%)",processes[i].mem_usage,umbral_RAM);
                strncat(text,temp,sizeof(text) - strlen(text) - 1);
            }
        }

        //Si debe generar alerta la imprime
        if(generate_alert){
            printf("🚨 [ALERTA] Proceso '%s' (PID: %d) - %s\n", 
                processes[i].name, processes[i].pid, text);
            alerts_founded++;

            //Escribe la alerta en el archivo
            char alert_message[1024];
            snprintf(alert_message, sizeof(alert_message), 
                "🚨 ALERTA: Proceso '%s' (PID: %d) - %s", 
                processes[i].name, processes[i].pid, text);
            Write_Alert("PROCESOS", alert_message);
            sleep(3); //Pausa para evitar spam de alertas
        }
    }

        if (alerts_founded == 0) {
            printf("✅ No se detectaron procesos sospechosos.\n");
        } else {
            printf("⚠️  Total de alertas: %d\n", alerts_founded);
        }
}

//Muestra un reporte ordenado de los procesos con mayor uso de CPU
void show_report(){
    printf("\n📊 === REPORTE DE PROCESOS ===\n");
    printf("%-8s %-20s %-10s %-10s %-15s\n", "PID", "NOMBRE" , "CPU%" , "MEM%", "Estado");
    printf("─────────────────────────────────────────────────────────────────────────\n");

    //Crea array temporal para ordenar
    Process temp[Max_Processes];
    int active = 0;             //Contador de procesos activos

    //Copia solo los procesos activos el array temporal
    for(int i = 0;i < num_processes;i++){
        if(processes[i].active && processes[i].pid > 0){
            temp[active] = processes[i];
            active++;
        }
    }

    //Ordena por uso de CPU
    for(int i = 0;i < active;i++){
        for(int j = i + 1;j < active;j++){
            if(temp[i].cpu_usage < temp[j].cpu_usage){
                Process aux = temp[j];
                temp[j] = temp[i];
                temp[i] = aux;
            }
        }
    }

    //Muestra solo los 20 top procesos
    int show = (active > 20) ? 20 : active;
    for(int i = 0;i < show;i++){
        char state[16];

        //Determina el estado basado en los umbrales
        if(temp[i].cpu_usage > umbral_CPU || temp[i].mem_usage > umbral_RAM){
            if(is_whitelist_process(temp[i].name)){
                strcpy(state,"Lista Blanca");
            }
            else{
                strcpy(state,"Sospechoso");
            }
        }
        else{
            strcpy(state,"Normal");
        }

        printf("%-8d %-20.20s %-10.1f %-10.1f %-15s\n",temp[i].pid,temp[i].name,temp[i].cpu_usage,temp[i].mem_usage,state);
    }
    printf("\n📈 Total de procesos activos: %d (mostrando top %d)\n", active,show);
}

//Coordina todo el proceso de monitoreo continuo
void process_handler(void){
    print_module();
    init_process();
    printf("🔄 Estableciendo baseline inicial...\n");
    scan_process();
    printf("⏳ Esperando %d segundos para calibrar...\n", monitor_Time);
    sleep(monitor_Time);

    int iteration = 1;
    //while(1){
        printf("\n🔍 --- ITERACIÓN %d ---\n", iteration);
        scan_process();
        show_report();
        check_alert();
        printf("\n⏰ Esperando %d segundos para próxima lectura...\n", monitor_Time);
        sleep(monitor_Time);
        iteration++;
    //}
}


//Funcion principal del programa
int main(){
    printf("🚀 Iniciando MatCom Guard - Módulo 2...\n");
    printf("⚠️  Nota: Se requieren permisos de administrador para acceso completo a /proc\n\n");
    load_config();
    process_handler();
    return 0;
}