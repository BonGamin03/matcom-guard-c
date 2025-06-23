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
#include "../../Scanners/util.h"
#include <pthread.h>

#define MAX_PATH 4096
#define MAX_DEVICES 100
#define MAX_FILES 10000
#define HASH_SIZE 33
#define THRESHOLD_PERCENTAGE 10
#define SCAN_INTERVAL 15  // Aumentado de 5 a 15 segundos
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

// Constantes para detectar crecimiento inusual
#define GROWTH_THRESHOLD_PERCENTAGE 200  // 200% = archivo creció al doble
#define GROWTH_THRESHOLD_BYTES 1048576   // 1MB de crecimiento súbito

// Estructuras de datos
typedef struct {
    char path[MAX_PATH];
    char hash[HASH_SIZE];
    time_t last_modified;
    off_t size;
    mode_t permissions;
    uid_t owner;
    gid_t group;
    char extension[32];  // Extensión del archivo
    int exists;          // Flag para marcar si el archivo existe
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
    int first_scan_done;  // Flag para saber si ya se hizo el primer escaneo
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
void extract_file_extension(const char *path, char *extension);
void analyze_file_modification(FileInfo *current, FileInfo *previous, const char *device_name);
void detect_file_replication(USBDevice *device);
int are_files_similar(FileInfo *file1, FileInfo *file2);
void copy_file_array(FileInfo *dest, FileInfo *src, int count);

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
        
        sleep(2); // Intervalo más corto para inotify pero el escaneo es cada 15 segundos
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
                    new_device->first_scan_done = 0;
                    
                    printf("🔍 Nueva amenaza potencial detectada: %s en %s\n", 
                           device, mount_point);
                    emit_alert(device, "NUEVO_DISPOSITIVO", mount_point);
                    
                    // Configurar inotify para monitoreo en tiempo real
                    setup_inotify(new_device);
                    
                    // Realizar escaneo inicial
                    printf("📊 Iniciando escaneo inicial...\n");
                    scan_device_files(new_device);
                    new_device->first_scan_done = 1;
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
            //snprintf(full_path, sizeof(full_path), "%s/%s", device->mount_point, event->name);
            snprintf(full_path, sizeof(full_path), device->mount_point, event->name);
            
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
        
        // Guardar estado anterior solo si ya hicimos al menos un escaneo
        if (device->first_scan_done) {
            copy_file_array(device->previous_files, device->files, device->file_count);
            device->previous_file_count = device->file_count;
        }
        
        // Nuevo escaneo
        device->file_count = 0;
        scan_device_files(device);
        
        // Detectar cambios específicos (solo si no es el primer escaneo)
        if (device->first_scan_done && (device->previous_file_count > 0 || device->file_count > 0)) {
            detect_file_changes(device);
        }
        
        device->last_scan = current_time;
        device->first_scan_done = 1;
    }
}

void copy_file_array(FileInfo *dest, FileInfo *src, int count) {
    if (!dest || !src) return;
    
    for (int i = 0; i < count && i < MAX_FILES; i++) {
        memcpy(&dest[i], &src[i], sizeof(FileInfo));
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
        //int ret = snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        int ret = snprintf(full_path, sizeof(full_path), dir_path, entry->d_name);
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
                file_info->exists = 1;
                
                // Extraer extensión del archivo
                extract_file_extension(full_path, file_info->extension);
                
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

void extract_file_extension(const char *path, char *extension) {
    if (!path || !extension) return;
    
    const char *dot = strrchr(path, '.');
    if (dot && dot != path) {
        strncpy(extension, dot + 1, 31);
        extension[31] = '\0';
        
        // Convertir a minúsculas
        for (int i = 0; extension[i]; i++) {
            if (extension[i] >= 'A' && extension[i] <= 'Z') {
                extension[i] = extension[i] + 32;
            }
        }
    } else {
        strcpy(extension, "");
    }
}

void detect_file_changes(USBDevice *device) {
    if (!device) return;
    
    printf("🔍 Analizando cambios en archivos...\n");
    
    // Detectar archivos añadidos
    detect_added_files(device);
    
    // Detectar archivos eliminados
    detect_deleted_files(device);
    
    // Detectar archivos modificados con análisis detallado
    detect_modified_files(device);
    
    // Detectar replicación de archivos
    detect_file_replication(device);
    
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
            snprintf(details, sizeof(details), "Archivo añadido: %s (Tamaño: %ld bytes, Ext: %s)", 
                    filename, device->files[i].size, device->files[i].extension);
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
            snprintf(details, sizeof(details), "Archivo eliminado: %s (Era de %ld bytes, Ext: %s)", 
                    filename, device->previous_files[i].size, device->previous_files[i].extension);
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
            const char *filename = get_filename_from_path(current->path);
            
            // 1. Cambio de tamaño
            if (current->size != previous->size) {
                if (current->size > previous->size) {
                    off_t growth = current->size - previous->size;
                    printf("📏 El tamaño del archivo \"%s\" ha aumentado de %ld a %ld bytes (+%ld bytes)\n", 
                           filename, previous->size, current->size, growth);
                } else {
                    off_t reduction = previous->size - current->size;
                    printf("📏 El tamaño del archivo \"%s\" ha disminuido de %ld a %ld bytes (-%ld bytes)\n", 
                           filename, previous->size, current->size, reduction);
                }
            }
            
            // 2. Cambio de extensión
            if (strcmp(current->extension, previous->extension) != 0) {
                if (strlen(previous->extension) == 0) {
                    printf("🔄 La extensión del archivo \"%s\" ha cambiado de (sin extensión) a .%s\n", 
                           filename, current->extension);
                } else if (strlen(current->extension) == 0) {
                    printf("🔄 La extensión del archivo \"%s\" ha cambiado de .%s a (sin extensión)\n", 
                           filename, previous->extension);
                } else {
                    printf("🔄 La extensión del archivo \"%s\" ha cambiado de .%s a .%s\n", 
                           filename, previous->extension, current->extension);
                }
            }
            
            // 3. Cambio de permisos
            if (current->permissions != previous->permissions) {
                printf("🔐 Los permisos del archivo \"%s\" han cambiado de %o a %o\n", 
                       filename, 
                       previous->permissions & 0777, current->permissions & 0777);
            }
            
            // 4. Cambio de contenido (hash)
            if (strcmp(current->hash, previous->hash) != 0) {
                printf("📝 El contenido del archivo \"%s\" ha sido modificado\n", filename);
            }
            
            // 5. Cambio de fecha de modificación
            if (current->last_modified != previous->last_modified) {
                char prev_time[64], curr_time[64];
                struct tm *prev_tm = localtime(&previous->last_modified);
                struct tm *curr_tm = localtime(&current->last_modified);
                
                strftime(prev_time, sizeof(prev_time), "%Y-%m-%d %H:%M:%S", prev_tm);
                strftime(curr_time, sizeof(curr_time), "%Y-%m-%d %H:%M:%S", curr_tm);
                
                printf("⏰ La fecha de modificación del archivo \"%s\" ha cambiado de %s a %s\n", 
                       filename, prev_time, curr_time);
            }
            
            // 6. Cambio de propietario
            if (current->owner != previous->owner || current->group != previous->group) {
                printf("👤 El propietario del archivo \"%s\" ha cambiado de UID:%d GID:%d a UID:%d GID:%d\n", 
                       filename, 
                       previous->owner, previous->group, current->owner, current->group);
            }
            
            // Si hubo cualquier cambio, hacer análisis detallado
            if (current->size != previous->size || 
                strcmp(current->extension, previous->extension) != 0 ||
                current->permissions != previous->permissions ||
                strcmp(current->hash, previous->hash) != 0 ||
                current->last_modified != previous->last_modified ||
                current->owner != previous->owner ||
                current->group != previous->group) {
                
                analyze_file_modification(current, previous, device->device_name);
            }
        }
    }
}

void analyze_file_modification(FileInfo *current, FileInfo *previous, const char *device_name) {
    if (!current || !previous || !device_name) return;
    
    const char *filename = get_filename_from_path(current->path);
    
    // 1. Verificar crecimiento inusual de tamaño
    if (current->size > previous->size) {
        off_t growth = current->size - previous->size;
        double growth_percentage = (double)growth / (previous->size > 0 ? previous->size : 1) * 100.0;
        
        if (growth_percentage > GROWTH_THRESHOLD_PERCENTAGE || growth > GROWTH_THRESHOLD_BYTES) {
            printf("🚨 ¡CRECIMIENTO INUSUAL! El archivo \"%s\" ha crecido de manera sospechosa\n", filename);
            
            char details[512];
            snprintf(details, sizeof(details), 
                    "CRECIMIENTO INUSUAL: El archivo \"%s\" creció %.1f%% (%ld -> %ld bytes)", 
                    filename, growth_percentage, previous->size, current->size);
            emit_alert(device_name, "CRECIMIENTO_INUSUAL", details);
        }
    }
    
    // 2. Verificar cambio de extensión
    if (strcmp(current->extension, previous->extension) != 0) {
        char details[512];
        if (strlen(previous->extension) == 0) {
            snprintf(details, sizeof(details), 
                    "CAMBIO DE EXTENSIÓN: El archivo \"%s\" ahora tiene extensión .%s", 
                    filename, current->extension);
        } else if (strlen(current->extension) == 0) {
            snprintf(details, sizeof(details), 
                    "CAMBIO DE EXTENSIÓN: El archivo \"%s\" perdió su extensión .%s", 
                    filename, previous->extension);
        } else {
            snprintf(details, sizeof(details), 
                    "CAMBIO DE EXTENSIÓN: El archivo \"%s\" cambió de .%s a .%s", 
                    filename, previous->extension, current->extension);
        }
        emit_alert(device_name, "CAMBIO_EXTENSION", details);
    }
    
    // 3. Verificar cambio de permisos
    if (current->permissions != previous->permissions) {
        char details[512];
        snprintf(details, sizeof(details), 
                "CAMBIO DE PERMISOS: El archivo \"%s\" cambió permisos de %o a %o", 
                filename, previous->permissions & 0777, current->permissions & 0777);
        emit_alert(device_name, "CAMBIO_PERMISOS", details);
    }
    
    // 4. Verificar cambio de contenido (hash diferente)
    if (strcmp(current->hash, previous->hash) != 0) {
        char details[512];
        snprintf(details, sizeof(details), 
                "CONTENIDO MODIFICADO: El archivo \"%s\" tiene nuevo contenido", 
                filename);
        emit_alert(device_name, "CONTENIDO_MODIFICADO", details);
    }
    
    // 5. Verificar cambio de propietario
    if (current->owner != previous->owner || current->group != previous->group) {
        char details[512];
        snprintf(details, sizeof(details), 
                "CAMBIO DE PROPIETARIO: El archivo \"%s\" cambió de UID:%d GID:%d a UID:%d GID:%d", 
                filename, previous->owner, previous->group, current->owner, current->group);
        emit_alert(device_name, "CAMBIO_PROPIETARIO", details);
    }
    
    // 6. Análisis de cambio de tamaño
    if (current->size != previous->size) {
        char details[512];
        if (current->size > previous->size) {
            off_t growth = current->size - previous->size;
            snprintf(details, sizeof(details), 
                    "TAMAÑO INCREMENTADO: El archivo \"%s\" creció %ld bytes (%ld -> %ld)", 
                    filename, growth, previous->size, current->size);
        } else {
            off_t reduction = previous->size - current->size;
            snprintf(details, sizeof(details), 
                    "TAMAÑO REDUCIDO: El archivo \"%s\" se redujo %ld bytes (%ld -> %ld)", 
                    filename, reduction, previous->size, current->size);
        }
        emit_alert(device_name, "CAMBIO_TAMAÑO", details);
    }
}

void detect_file_replication(USBDevice *device) {
    if (!device) return;
    
    printf("🔍 Detectando replicación de archivos...\n");
    
    // Comparar archivos actuales para encontrar duplicados sospechosos
    for (int i = 0; i < device->file_count - 1; i++) {
        for (int j = i + 1; j < device->file_count; j++) {
            if (are_files_similar(&device->files[i], &device->files[j])) {
                const char *filename1 = get_filename_from_path(device->files[i].path);
                const char *filename2 = get_filename_from_path(device->files[j].path);
                
                printf("👥 ¡REPLICACIÓN DETECTADA! Los archivos \"%s\" y \"%s\" son idénticos\n", filename1, filename2);
                
                char details[512];
                snprintf(details, sizeof(details), 
                        "REPLICACIÓN: Archivos idénticos detectados: \"%s\" (%ld bytes) y \"%s\" (%ld bytes)", 
                        filename1, device->files[i].size, filename2, device->files[j].size);
                emit_alert(device->device_name, "REPLICACION_ARCHIVOS", details);
            }
        }
    }
}

int are_files_similar(FileInfo *file1, FileInfo *file2) {
    if (!file1 || !file2) return 0;
    
    // Archivos similares si:
    // 1. Tienen el mismo tamaño
    // 2. Tienen el mismo hash (mismo contenido)
    // 3. Pero nombres diferentes
    return (file1->size == file2->size && 
            strcmp(file1->hash, file2->hash) == 0 &&
            strcmp(file1->path, file2->path) != 0);
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

    //Escribe la alerta en el archivo
    char alert_message[512];
    snprintf(alert_message, sizeof(alert_message), 
            "🚨 ALERTA USB: [%s] %s en %s - %s", 
            time_str ? time_str : "UNKNOWN", alert_type, device_name, details);
    Write_Alert(alert_message);
    sleep(5); //Pausa para evitar spam de alertas
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