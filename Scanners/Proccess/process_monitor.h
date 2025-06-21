#ifndef Process_Monitor_H
#define Process_Monitor_

//Constantes para limites del sistema
#define Max_Processes 1000

//Estructura para almacenar informacion de un proceso
typedef struct{
    int pid;                            //ID del Proceso
    char name[256];                     //Nombre del Proceso
    unsigned long last_user_time;       //Tiempo de CPU en modo usuario
    unsigned long last_system_time;     //Tiempo de CPU en modo sistema
    unsigned long last_total_time;      //Tiempo total de CPU
    double cpu_usage;                   //Porcentaje de uso de CPU calculado
    double mem_usage;                   //Porcentaje de uso de memoria calculado
    int active;                         //Indica si el proceso sigue activo(1 = activo , 0 = inactivo)
}Process;

//Declaraciones de funciones principales
void load_config(void);                            //Carga la configuracion inicial
int is_whitelist_process(const char* name);    //Verifica si un proceso esta en la lista blanca
unsigned long get_total_CPU(void);             //Obtiene el tiempo total de CPU del sistema
unsigned long get_total_memory(void);          //Obtiene la cantidad total de memoria del sistema
int read_process(int pid,Process* process);    //Lee la informacion de un proceso
void scan_process(void);                       //Escanea los procesos del sistema 
void check_alert(void);                        //Verifica si un proceso supera los umbrales
void show_report(void);                        //Muestra un reporte de los procesos
void process_handler(void);                    //Funcion principal de monitoreo de procesos
void print_module(void);                       //Para imprimir y probar los test
void init_process(void);                       //Inicializa el array de procesos

#endif