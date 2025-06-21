#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/inotify.h>

#define MAX_PATH 4096
#define MAX_DEVICES 100
#define MAX_FILES 10000
#define HASH_SIZE 33
#define THRESHOLD_PERCENTAGE 10
#define SCAN_INTERVAL 5
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

// Estructuras de datos
typedef struct {
    char path[MAX_PATH];
    char hash[HASH_SIZE];
    time_t last_modified;
    off_t size;
    mode_t permissions;
    uid_t owner;
    gid_t group;
} FileInfo;

typedef struct {
    char mount_point[MAX_PATH];
    char device_name[256];
    FileInfo files[MAX_FILES];
    FileInfo previous_files[MAX_FILES];  // Estado anterior para comparación
    int file_count;
    int previous_file_count;
    time_t last_scan;
    int is_monitored;
    int inotify_fd;
    int watch_descriptor;
} USBDevice;

typedef struct {
    USBDevice devices[MAX_DEVICES];
    int device_count;
    int running;
    int threshold;
} GuardSystem;

// Variables globales
static GuardSystem guard = {0};

// Prototipos de funciones
void init_guard_system(void);
void print_banner(void);
void scan_mount_points(void);
void monitor_device(USBDevice *device);
void scan_device_files(USBDevice *device);
int calculate_file_hash(const char *filepath, char *hash_output);
void detect_file_changes(USBDevice *device);
void emit_alert(const char *device_name, const char *alert_type, const char *details);
void cleanup_and_exit(int sig);
void scan_directory_recursive(const char *dir_path, USBDevice *device);
int is_usb_device(const char *mount_point);
void log_event(const char *message);
void setup_inotify(USBDevice *device);
void handle_inotify_events(USBDevice *device);
void detect_added_files(USBDevice *device);
void detect_deleted_files(USBDevice *device);
void detect_modified_files(USBDevice *device);
int find_file_in_array(FileInfo *files, int count, const char *path);
const char* get_filename_from_path(const char *path);

// Función principal
int main(int argc, char *argv[]) {
    // Configurar manejador de señales
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);
    
    print_banner();
    init_guard_system();
    
    printf("🛡️  MatCom Guard iniciado - Patrullando las fronteras del reino...\n");
    printf("⚙️  Umbral de alerta: %d%% de archivos modificados\n", guard.threshold);
    printf("🔄 Intervalo de escaneo: %d segundos\n\n", SCAN_INTERVAL);
    
    // Ciclo principal de monitoreo
    while (guard.running) {
        scan_mount_points();
        
        // Monitorear dispositivos activos
        for (int i = 0; i < guard.device_count; i++) {
            if (guard.devices[i].is_monitored) {
                monitor_device(&guard.devices[i]);
                // Verificar eventos de inotify para cambios inmediatos
                handle_inotify_events(&guard.devices[i]);
            }
        }
        
        sleep(1); // Reducir intervalo para mejor respuesta de inotify
    }
    
    return 0;
}

void print_banner(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                        MATCOM GUARD                          ║\n");
    printf("║                   Guardián del Reino Digital                 ║\n");
    printf("║                                                               ║\n");
    printf("║    🏰 Protector de Fronteras - Detector de Intrusos 🏰      ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

void init_guard_system(void) {
    guard.device_count = 0;
    guard.running = 1;
    guard.threshold = THRESHOLD_PERCENTAGE;
    
    log_event("Sistema MatCom Guard inicializado");
}

void scan_mount_points(void) {
    FILE *mounts = fopen("/proc/mounts", "r");
    if (!mounts) {
        perror("Error al abrir /proc/mounts");
        return;
    }
    
    char line[MAX_PATH];
    while (fgets(line, sizeof(line), mounts)) {
        char device[MAX_PATH], mount_point[MAX_PATH], fs_type[64];
        
        if (sscanf(line, "%s %s %s", device, mount_point, fs_type) == 3) {
            // Verificar si es un dispositivo USB
            if (is_usb_device(mount_point)) {
                
                // Buscar si ya está siendo monitoreado
                int found = 0;
                for (int i = 0; i < guard.device_count; i++) {
                    if (strcmp(guard.devices[i].mount_point, mount_point) == 0) {
                        found = 1;
                        break;
                    }
                }
                
                // Si es nuevo, agregarlo al sistema
                if (!found && guard.device_count < MAX_DEVICES) {
                    USBDevice *new_device = &guard.devices[guard.device_count];
                    
                    // Inicializar completamente la estructura
                    memset(new_device, 0, sizeof(USBDevice));
                    
                    strncpy(new_device->mount_point, mount_point, sizeof(new_device->mount_point) - 1);
                    strncpy(new_device->device_name, device, sizeof(new_device->device_name) - 1);
                    new_device->file_count = 0;
                    new_device->previous_file_count = 0;
                    new_device->last_scan = 0;
                    new_device->is_monitored = 1;
                    new_device->inotify_fd = -1;
                    new_device->watch_descriptor = -1;
                    
                    printf("🔍 Nueva amenaza potencial detectada: %s en %s\n", 
                           device, mount_point);
                    emit_alert(device, "NUEVO_DISPOSITIVO", mount_point);
                    
                    // Configurar inotify para monitoreo en tiempo real
                    setup_inotify(new_device);
                    
                    // Realizar escaneo inicial con protección
                    printf("📊 Iniciando escaneo inicial...\n");
                    scan_device_files(new_device);
                    guard.device_count++;
                }
            }
        }
    }
    
    fclose(mounts);
}

void setup_inotify(USBDevice *device) {
    if (!device) return;
    
    device->inotify_fd = inotify_init1(IN_NONBLOCK);
    if (device->inotify_fd == -1) {
        perror("inotify_init1");
        return;
    }
    
    device->watch_descriptor = inotify_add_watch(device->inotify_fd, 
                                                device->mount_point,
                                                IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM);
    if (device->watch_descriptor == -1) {
        perror("inotify_add_watch");
        close(device->inotify_fd);
        device->inotify_fd = -1;
        return;
    }
    
    printf("🔔 Monitoreo en tiempo real activado para %s\n", device->mount_point);
}

void handle_inotify_events(USBDevice *device) {
    if (!device || device->inotify_fd == -1) return;
    
    char buffer[BUF_LEN];
    int length = read(device->inotify_fd, buffer, BUF_LEN);
    
    if (length < 0) {
        if (errno != EAGAIN) {
            perror("read inotify");
        }
        return;
    }
    
    int i = 0;
    while (i < length) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];
        
        if (event->len > 0) {
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path),device->mount_point, event->name);
            
            /*
            #define MAX_PATH_LEN 4096
            char full_path[MAX_PATH_LEN];

            // Limitar los componentes para evitar overflow
            size_t mount_len = strlen(device->mount_point);
            size_t name_len = strlen(event->name);
            size_t max_name = MAX_PATH_LEN - mount_len - 2; // -2 por '/' y '\0'

            char safe_name[MAX_PATH_LEN];
            if (name_len > max_name) {
                strncpy(safe_name, event->name, max_name);
                safe_name[max_name] = '\0';
            } else {
                strcpy(safe_name, event->name);
            }

            int written = snprintf(full_path, sizeof(full_path), "%s/%s", device->mount_point, safe_name);
            if (written < 0 || written >= MAX_PATH_LEN) {
            fprintf(stderr, "Advertencia: la ruta fue truncada: %s/%s\n", device->mount_point, event->name);
            }
            */


            if (event->mask & IN_CREATE) {
                printf("📁 ¡ARCHIVO AÑADIDO! %s\n", event->name);
                emit_alert(device->device_name, "ARCHIVO_AÑADIDO", event->name);
            }
            
            if (event->mask & IN_DELETE) {
                printf("🗑️  ¡ARCHIVO ELIMINADO! %s\n", event->name);
                emit_alert(device->device_name, "ARCHIVO_ELIMINADO", event->name);
            }
            
            if (event->mask & IN_MODIFY) {
                printf("✏️  ¡ARCHIVO MODIFICADO! %s\n", event->name);
                emit_alert(device->device_name, "ARCHIVO_MODIFICADO", event->name);
            }
            
            if (event->mask & IN_MOVED_TO) {
                printf("📥 ¡ARCHIVO MOVIDO HACIA AQUÍ! %s\n", event->name);
                emit_alert(device->device_name, "ARCHIVO_MOVIDO_ENTRADA", event->name);
            }
            
            if (event->mask & IN_MOVED_FROM) {
                printf("📤 ¡ARCHIVO MOVIDO DESDE AQUÍ! %s\n", event->name);
                emit_alert(device->device_name, "ARCHIVO_MOVIDO_SALIDA", event->name);
            }
        }
        
        i += EVENT_SIZE + event->len;
    }
}

int is_usb_device(const char *mount_point) {
    if (!mount_point) return 0;
    
    // Verificar si el punto de montaje indica un dispositivo USB
    return (strstr(mount_point, "/media/") != NULL ||
            strstr(mount_point, "/mnt/") != NULL ||
            strstr(mount_point, "/run/media/") != NULL);
}

void monitor_device(USBDevice *device) {
    if (!device) return;
    
    // Verificar si el dispositivo sigue montado
    if (access(device->mount_point, F_OK) != 0) {
        printf("📤 Dispositivo %s desconectado\n", device->device_name);
        device->is_monitored = 0;
        
        // Limpiar inotify
        if (device->inotify_fd != -1) {
            close(device->inotify_fd);
            device->inotify_fd = -1;
        }
        return;
    }
    
    time_t current_time = time(NULL);
    if (current_time - device->last_scan >= SCAN_INTERVAL) {
        printf("🔍 Patrullando %s...\n", device->mount_point);
        
        // Guardar estado anterior
        memcpy(device->previous_files, device->files, sizeof(device->files));
        device->previous_file_count = device->file_count;
        
        // Nuevo escaneo
        device->file_count = 0;
        scan_device_files(device);
        
        // Detectar cambios específicos
        if (device->previous_file_count > 0 || device->file_count > 0) {
            detect_file_changes(device);
        }
        
        device->last_scan = current_time;
    }
}

void scan_device_files(USBDevice *device) {
    if (!device) return;
    
    device->file_count = 0;
    scan_directory_recursive(device->mount_point, device);
    
    printf("📊 Escaneados %d archivos en %s\n", 
           device->file_count, device->mount_point);
}

void scan_directory_recursive(const char *dir_path, USBDevice *device) {
    if (!dir_path || !device || device->file_count >= MAX_FILES) return;
    
    DIR *dir = opendir(dir_path);
    if (!dir) return;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && device->file_count < MAX_FILES) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[MAX_PATH];
        int ret = snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (ret >= sizeof(full_path)) {
            continue; // Path too long
        }
        
        struct stat file_stat;
        if (stat(full_path, &file_stat) == 0) {
            if (S_ISREG(file_stat.st_mode)) {
                // Es un archivo regular
                FileInfo *file_info = &device->files[device->file_count];
                
                // Inicializar la estructura
                memset(file_info, 0, sizeof(FileInfo));
                
                strncpy(file_info->path, full_path, sizeof(file_info->path) - 1);
                file_info->last_modified = file_stat.st_mtime;
                file_info->size = file_stat.st_size;
                file_info->permissions = file_stat.st_mode;
                file_info->owner = file_stat.st_uid;
                file_info->group = file_stat.st_gid;
                
                // Calcular hash del archivo
                if (calculate_file_hash(full_path, file_info->hash) == 0) {
                    device->file_count++;
                }
            } else if (S_ISDIR(file_stat.st_mode)) {
                // Es un directorio, escanear recursivamente
                scan_directory_recursive(full_path, device);
            }
        }
    }
    
    closedir(dir);
}

void detect_file_changes(USBDevice *device) {
    if (!device) return;
    
    printf("🔍 Analizando cambios en archivos...\n");
    
    // Detectar archivos añadidos
    detect_added_files(device);
    
    // Detectar archivos eliminados
    detect_deleted_files(device);
    
    // Detectar archivos modificados
    detect_modified_files(device);
    
    printf("✅ Análisis de cambios completado\n");
}

void detect_added_files(USBDevice *device) {
    if (!device) return;
    
    for (int i = 0; i < device->file_count; i++) {
        // Buscar si este archivo existía en el escaneo anterior
        int found = find_file_in_array(device->previous_files, device->previous_file_count, 
                                      device->files[i].path);
        
        if (found == -1) {
            // Es un archivo nuevo
            const char *filename = get_filename_from_path(device->files[i].path);
            printf("📁 ¡NUEVO ARCHIVO DETECTADO! %s\n", filename);
            
            char details[512];
            snprintf(details, sizeof(details), "Archivo añadido: %s (Tamaño: %ld bytes)", 
                    filename, device->files[i].size);
            emit_alert(device->device_name, "ARCHIVO_AÑADIDO", details);
        }
    }
}

void detect_deleted_files(USBDevice *device) {
    if (!device) return;
    
    for (int i = 0; i < device->previous_file_count; i++) {
        // Buscar si este archivo sigue existiendo
        int found = find_file_in_array(device->files, device->file_count, 
                                      device->previous_files[i].path);
        
        if (found == -1) {
            // El archivo fue eliminado
            const char *filename = get_filename_from_path(device->previous_files[i].path);
            printf("🗑️  ¡ARCHIVO ELIMINADO DETECTADO! %s\n", filename);
            
            char details[512];
            snprintf(details, sizeof(details), "Archivo eliminado: %s (Era de %ld bytes)", 
                    filename, device->previous_files[i].size);
            emit_alert(device->device_name, "ARCHIVO_ELIMINADO", details);
        }
    }
}

void detect_modified_files(USBDevice *device) {
    if (!device) return;
    
    for (int i = 0; i < device->file_count; i++) {
        // Buscar el archivo en el escaneo anterior
        int prev_index = find_file_in_array(device->previous_files, device->previous_file_count, 
                                           device->files[i].path);
        
        if (prev_index != -1) {
            // El archivo existía antes, verificar si cambió
            FileInfo *current = &device->files[i];
            FileInfo *previous = &device->previous_files[prev_index];
            
            if (current->last_modified != previous->last_modified ||
                current->size != previous->size ||
                strcmp(current->hash, previous->hash) != 0) {
                
                const char *filename = get_filename_from_path(current->path);
                printf("✏️  ¡ARCHIVO MODIFICADO DETECTADO! %s\n", filename);
                
                char details[512];
                snprintf(details, sizeof(details), 
                        "Archivo modificado: %s (Tamaño: %ld->%ld bytes)", 
                        filename, previous->size, current->size);
                emit_alert(device->device_name, "ARCHIVO_MODIFICADO", details);
            }
        }
    }
}

int find_file_in_array(FileInfo *files, int count, const char *path) {
    if (!files || !path) return -1;
    
    for (int i = 0; i < count; i++) {
        if (strcmp(files[i].path, path) == 0) {
            return i;
        }
    }
    return -1;
}

const char* get_filename_from_path(const char *path) {
    if (!path) return "unknown";
    
    const char *filename = strrchr(path, '/');
    return filename ? filename + 1 : path;
}

int calculate_file_hash(const char *filepath, char *hash_output) {
    if (!filepath || !hash_output) return -1;
    
    FILE *file = fopen(filepath, "rb");
    if (!file) return -1;
    
    // Hash simple basado en tamaño, primera parte y última parte del archivo
    struct stat st;
    if (fstat(fileno(file), &st) != 0) {
        fclose(file);
        return -1;
    }
    
    unsigned long hash = st.st_size;
    unsigned char buffer[1024];
    size_t bytes_read;
    
    // Leer primeros 512 bytes
    bytes_read = fread(buffer, 1, 512, file);
    for (size_t i = 0; i < bytes_read; i++) {
        hash = hash * 31 + buffer[i];
    }
    
    // Si el archivo es grande, leer últimos 512 bytes
    if (st.st_size > 1024) {
        if (fseek(file, -512, SEEK_END) == 0) {
            bytes_read = fread(buffer, 1, 512, file);
            for (size_t i = 0; i < bytes_read; i++) {
                hash = hash * 31 + buffer[i];
            }
        }
    }
    
    // Convertir a string hexadecimal
    snprintf(hash_output, HASH_SIZE, "%08lx", hash);
    
    fclose(file);
    return 0;
}

void emit_alert(const char *device_name, const char *alert_type, const char *details) {
    if (!device_name || !alert_type || !details) return;
    
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    if (time_str) {
        time_str[strlen(time_str) - 1] = '\0'; // Remover salto de línea
    }
    
    printf("🚨 ALERTA: [%s] %s en %s - %s\n", 
           time_str ? time_str : "UNKNOWN", alert_type, device_name, details);
    
    // Log a archivo
    char log_message[1024];
    snprintf(log_message, sizeof(log_message), 
            "ALERTA: [%s] %s en %s - %s", 
            time_str ? time_str : "UNKNOWN", alert_type, device_name, details);
    log_event(log_message);
}

void log_event(const char *message) {
    if (!message) return;
    
    // Crear directorio si no existe
    system("mkdir -p /tmp/matcom_guard_logs");
    
    FILE *log_file = fopen("/tmp/matcom_guard_logs/guard.log", "a");
    if (log_file) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        if (time_str) {
            time_str[strlen(time_str) - 1] = '\0';
        }
        
        fprintf(log_file, "[%s] %s\n", time_str ? time_str : "UNKNOWN", message);
        fclose(log_file);
    }
}

void cleanup_and_exit(int sig) {
    printf("\n🛑 Deteniendo MatCom Guard...\n");
    guard.running = 0;
    
    // Cerrar descriptores de inotify
    for (int i = 0; i < guard.device_count; i++) {
        if (guard.devices[i].inotify_fd != -1) {
            close(guard.devices[i].inotify_fd);
        }
    }
    
    log_event("Sistema MatCom Guard detenido");
    exit(0);
}