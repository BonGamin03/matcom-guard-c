#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <openssl/evp.h>
#include <mntent.h>
#include <sys/inotify.h>
#include <errno.h>
#include <signal.h>
#include "../../Scanners/util.h"

#define MAX_PATH 4096
#define MAX_FILES 100
#define HASH_SIZE 65
#define BUFFER_SIZE 8192
#define UMBRAL_CAMBIOS 10 // Porcentaje de archivos que pueden cambiar antes de alertar

// Estructura para almacenar información de archivos
typedef struct {
    char path[MAX_PATH];
    char hash[HASH_SIZE];
    off_t size;
    time_t mtime;
    mode_t permissions;
    uid_t owner;
    gid_t group;
    int exists;
} FileInfo;

// Estructura para dispositivos USB
typedef struct {
    char mount_point[MAX_PATH];
    char device[MAX_PATH];
    FileInfo files[MAX_FILES];
    int file_count;
    int is_monitored;
} USBDevice;

static USBDevice usb_devices[10];
static int device_count = 0;
static volatile int running = 1;

// Función para manejar señales de terminación
void signal_handler(int sig) {
    printf("\n[REINO] Deteniendo vigilancia por orden real...\n");
    running = 0;
}

// Función para calcular hash SHA-256 de un archivo (OpenSSL 3.0+)
int calculate_file_hash(const char *filepath, char *hash_output) {
    if (!filepath || !hash_output) {
        if (hash_output) strcpy(hash_output, "ERROR_PARAM");
        return 0;
    }
    
    struct stat st;
    if (stat(filepath, &st) != 0) {
        strcpy(hash_output, "ERROR_STAT");
        return 0;
    }
    
    uint64_t hash = 0;
    
    // Combinar metadatos
    hash ^= (uint64_t)st.st_size;
    hash ^= (uint64_t)st.st_mtime << 16;
    hash ^= (uint64_t)st.st_ino << 32;
    
    // Agregar muestra del contenido para archivos regulares pequeños
    if (S_ISREG(st.st_mode) && st.st_size > 0 && st.st_size < 10*1024*1024) {
        FILE *file = fopen(filepath, "rb");
        if (file) {
            unsigned char buffer[1024];
            size_t bytes_read;
            
            // Leer muestra del inicio
            bytes_read = fread(buffer, 1, sizeof(buffer), file);
            for (size_t i = 0; i < bytes_read; i++) {
                hash = ((hash << 5) + hash) + buffer[i];
            }
            
            // Si es grande, leer también del final
            if (st.st_size > sizeof(buffer)) {
                fseek(file, -sizeof(buffer), SEEK_END);
                bytes_read = fread(buffer, 1, sizeof(buffer), file);
                for (size_t i = 0; i < bytes_read; i++) {
                    hash = ((hash << 3) + hash) + buffer[i];
                }
            }
            
            fclose(file);
        }
    }
    
    // Convertir a string hexadecimal
    snprintf(hash_output, HASH_SIZE-1, "%016lx", hash);
    return 1;
}
// Función para obtener información de un archivo
int get_file_info(const char *filepath, FileInfo *info) {
    struct stat st;
    if (stat(filepath, &st) != 0) {
        info->exists = 0;
        return 0;
    }
    
    strncpy(info->path, filepath, MAX_PATH - 1);
    info->path[MAX_PATH - 1] = '\0';
    info->size = st.st_size;
    info->mtime = st.st_mtime;
    info->permissions = st.st_mode;
    info->owner = st.st_uid;
    info->group = st.st_gid;
    info->exists = 1;
    
    // Calcular hash solo para archivos regulares
    if (S_ISREG(st.st_mode)) {
        if (!calculate_file_hash(filepath, info->hash)) {
            strcpy(info->hash, "ERROR");
        }
    } else {
        strcpy(info->hash, "NOT_FILE");
    }
    
    return 1;
}

// Función recursiva para escanear directorio
int scan_directory(const char *dir_path, FileInfo *files, int *file_count, int max_files) {
    DIR *dir = opendir(dir_path);
    if (!dir) return -1;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && *file_count < max_files) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Recursión para subdirectorios
                scan_directory(full_path, files, file_count, max_files);
            } else {
                // Agregar archivo a la lista
                get_file_info(full_path, &files[*file_count]);
                (*file_count)++;
            }
        }
    }
    
    closedir(dir);
    return 1;
}

// Función para detectar dispositivos USB montados
int detect_usb_devices() {
    FILE *mounts = setmntent("/proc/mounts", "r");
    if (!mounts) {
        perror("Error abriendo /proc/mounts");
        return 0;
    }
    
    int new_devices = 0;
    struct mntent *mount;
    
    while ((mount = getmntent(mounts)) != NULL) {
        // Buscar dispositivos USB (normalmente en /media, /mnt, o dispositivos sd*)
        if (strstr(mount->mnt_fsname, "/dev/sd") || 
            strstr(mount->mnt_dir, "/media") || 
            strstr(mount->mnt_dir, "/mnt")) {
            
            // Verificar si ya está siendo monitoreado
            int already_monitored = 0;
            for (int i = 0; i < device_count; i++) {
                if (strcmp(usb_devices[i].mount_point, mount->mnt_dir) == 0) {
                    already_monitored = 1;
                    break;
                }
            }
            
            if (!already_monitored && device_count < 10) {
                printf("\n🚨 [REINO] ¡ALERTA! Nuevo viajero detectado en las fronteras!\n");
                printf("📍 Punto de entrada: %s\n", mount->mnt_dir);
                printf("🛡️  Dispositivo: %s\n", mount->mnt_fsname);
                printf("⚔️  Iniciando escaneo de seguridad...\n\n");

                //Escribir alerta de nuevo dispositivo
                char alert_message[512];
                snprintf(alert_message, sizeof(alert_message), 
                         "🚨 ALERTA NUEVO DISPOSITIVO: %s montado en %s", 
                         mount->mnt_fsname, mount->mnt_dir);
                Write_Alert("USB", alert_message);
                sleep(3); // Pausa para evitar spam de alertas                         

                // Agregar nuevo dispositivo USB
                strncpy(usb_devices[device_count].mount_point, mount->mnt_dir, MAX_PATH - 1);
                strncpy(usb_devices[device_count].device, mount->mnt_fsname, MAX_PATH - 1);
                usb_devices[device_count].file_count = 0;
                usb_devices[device_count].is_monitored = 1;
                
                // Crear baseline inicial
                scan_directory(mount->mnt_dir, usb_devices[device_count].files, 
                             &usb_devices[device_count].file_count, MAX_FILES);
                
                printf("📜 Baseline creado: %d archivos catalogados\n", 
                       usb_devices[device_count].file_count);
                
                device_count++;
                new_devices++;
            }
        }
    }
    
    endmntent(mounts);
    return new_devices;
}

// Función para detectar cambios sospechosos
void analyze_changes(USBDevice *device) {
    printf("\n🔍 [REINO] Analizando cambios en: %s\n", device->mount_point);
    
    FileInfo* current_files = malloc(MAX_FILES * sizeof(FileInfo));
    int current_count = 0;
    
    // Escanear estado actual
    scan_directory(device->mount_point, current_files, &current_count, MAX_FILES);
    
    int files_changed = 0;
    int files_added = 0;
    int files_deleted = 0;
    int suspicious_changes = 0;
    
    // Comparar archivos actuales con baseline
    for (int i = 0; i < current_count; i++) {
        int found_in_baseline = 0;
        
        for (int j = 0; j < device->file_count; j++) {
            if (strcmp(current_files[i].path, device->files[j].path) == 0) {
                found_in_baseline = 1;
                
                // Verificar cambios sospechosos
                if (strcmp(current_files[i].hash, device->files[j].hash) != 0) {
                    files_changed++;
                    
                    // Cambio de tamaño drástico
                    if (current_files[i].size > device->files[j].size * 10) {
                        printf("⚠️  [TRAICIÓN] Crecimiento inusual detectado:\n");
                        printf("   📁 %s\n", current_files[i].path);
                        printf("   📊 Tamaño: %ld → %ld bytes\n", 
                               device->files[j].size, current_files[i].size);
                        suspicious_changes++;

                        //Escribir alerta
                        char alert_message[512];
                        snprintf(alert_message, sizeof(alert_message), 
                                 "⚠️ ALERTA TRAICIÓN: Crecimiento inusual en %s: %ld → %ld bytes", 
                                 current_files[i].path, device->files[j].size, current_files[i].size);
                        Write_Alert("USB", alert_message);
                        sleep(3); // Pausa para evitar spam de alertas

                    }
                    
                    // Cambio de permisos
                   
                    mode_t permission_mask = S_IRWXU | S_IRWXG | S_IRWXO;  // Máscara para bits de permisos

                    if ((current_files[i].permissions & permission_mask) != 
                            (device->files[j].permissions & permission_mask)) 
                    {
                        printf("⚠️  [TRAICIÓN] Permisos modificados:\n");
                        printf("   📁 %s\n", current_files[i].path);
                        printf("   🔐 Permisos: %o → %o\n", 
                            device->files[j].permissions & permission_mask, 
                            current_files[i].permissions & permission_mask);
                        suspicious_changes++;
                        
                        //Escribir alerta
                        char alert_message[512];
                        snprintf(alert_message, sizeof(alert_message), 
                                 "⚠️ ALERTA TRAICIÓN: Permisos modificados en %s: %o → %o", 
                                 current_files[i].path, 
                                 device->files[j].permissions & permission_mask, 
                                 current_files[i].permissions & permission_mask);
                        Write_Alert("USB", alert_message);
                        sleep(3); // Pausa para evitar spam de alertas
                    }
                    // Cambio de propietario
                    if (current_files[i].owner != device->files[j].owner) {
                        printf("⚠️  [TRAICIÓN] Propietario cambiado:\n");
                        printf("   📁 %s\n", current_files[i].path);
                        printf("   👤 UID: %d → %d\n", 
                               device->files[j].owner, current_files[i].owner);
                        suspicious_changes++;

                        //Escribir alerta
                        char alert_message[512];
                        snprintf(alert_message, sizeof(alert_message), 
                                 "⚠️ ALERTA TRAICIÓN: Propietario cambiado en %s: %d → %d", 
                                 current_files[i].path, 
                                 device->files[j].owner, 
                                 current_files[i].owner);
                        Write_Alert("USB", alert_message);
                        sleep(3); // Pausa para evitar spam de alertas
                    }
                }
                break;
            }
        }
        
        // Archivo nuevo detectado
        if (!found_in_baseline) {
            files_added++;
            printf("✅ [REINO] Nuevo archivo detectado:\n");
            printf("   📁 %s\n", current_files[i].path);
            printf("   📊 Tamaño: %ld bytes\n", current_files[i].size);
            
            // Verificar si es sospechoso (archivos ejecutables o con extensiones raras)
            char *ext = strrchr(current_files[i].path, '.');
            if (ext && (strcmp(ext, ".exe") == 0 || strcmp(ext, ".scr") == 0 || 
                       strcmp(ext, ".bat") == 0 || strcmp(ext, ".com") == 0)) {
                printf("🚨 [ALERTA] ¡Archivo ejecutable sospechoso!\n");
                suspicious_changes++;

                //Escribir alerta
                char alert_message[512];
                snprintf(alert_message, sizeof(alert_message), 
                         "🚨 ALERTA: Archivo ejecutable sospechoso detectado: %s", 
                         current_files[i].path);
                Write_Alert("USB", alert_message);
                sleep(3); // Pausa para evitar spam de alertas
            }
        }
    }
    
    // Verificar archivos eliminados
    for (int i = 0; i < device->file_count; i++) {
        int found_in_current = 0;
        
        for (int j = 0; j < current_count; j++) {
            if (strcmp(device->files[i].path, current_files[j].path) == 0) {
                found_in_current = 1;
                break;
            }
        }
        
        if (!found_in_current) {
            files_deleted++;
            printf("❌ [REINO] Archivo eliminado detectado:\n");
            printf("   📁 %s\n", device->files[i].path);
            printf("   📊 Tamaño original: %ld bytes\n", device->files[i].size);

            //Escribir alerta
            char alert_message[512];
            snprintf(alert_message, sizeof(alert_message), 
                     "❌ ALERTA: Archivo eliminado detectado: %s (Tamaño original: %ld bytes)", 
                     device->files[i].path, device->files[i].size);
            Write_Alert("USB", alert_message);
            sleep(3); // Pausa para evitar spam de alertas
        }
    }
    
    // Calcular porcentaje de cambios
    int total_files = device->file_count > current_count ? device->file_count : current_count;
    int change_percentage = total_files > 0 ? ((files_changed + files_added + files_deleted) * 100) / total_files : 0;
    
    printf("\n📊 [REINO] Resumen del análisis:\n");
    printf("   📝 Archivos modificados: %d\n", files_changed);
    printf("   ➕ Archivos agregados: %d\n", files_added);
    printf("   ➖ Archivos eliminados: %d\n", files_deleted);
    printf("   📈 Porcentaje de cambios: %d%%\n", change_percentage);
    printf("   🚨 Cambios sospechosos: %d\n", suspicious_changes);
    
    // Alertar si supera el umbral
    if (change_percentage > UMBRAL_CAMBIOS || suspicious_changes > 0) {
        printf("\n🚨🚨🚨 [ALERTA MÁXIMA] 🚨🚨🚨\n");
        printf("¡POSIBLE INFILTRACIÓN O PLAGA DETECTADA!\n");
        printf("Se recomienda investigación inmediata del dispositivo.\n");
        printf("🛡️  Consideraciones de seguridad activadas.\n\n");

        // Escribir alerta de infiltración
        char alert_message[512];
        snprintf(alert_message, sizeof(alert_message), 
                 "🚨 ALERTA MÁXIMA: Posible infiltración detectada en %s. "
                 "Porcentaje de cambios: %d%%, Cambios sospechosos: %d", 
                 device->mount_point, change_percentage, suspicious_changes);
        Write_Alert("USB", alert_message);
        sleep(3); // Pausa para evitar spam de alertas
    }
    
    // Actualizar baseline
    memcpy(device->files, current_files, sizeof(FileInfo) * current_count);
    device->file_count = current_count;
    free(current_files);
}

// Función principal de monitoreo
void monitor_usb_devices() {
    printf("🏰 [REINO] Iniciando vigilancia de fronteras USB...\n");
    printf("⚔️  Presiona Ctrl+C para detener la vigilancia\n\n");
    
    while (running) {
        // Detectar nuevos dispositivos
        detect_usb_devices();
        
        // Analizar dispositivos existentes
        for (int i = 0; i < device_count; i++) {
            if (usb_devices[i].is_monitored) {
                // Verificar si el dispositivo sigue montado
                DIR *dir = opendir(usb_devices[i].mount_point);
                if (dir) {
                    closedir(dir);
                    analyze_changes(&usb_devices[i]);
                } else {
                    printf("📤 [REINO] Viajero se ha retirado: %s\n", 
                           usb_devices[i].mount_point);
                    usb_devices[i].is_monitored = 0;
                }
            }
        }
        
        // Esperar antes del próximo ciclo de monitoreo
        printf("💤 [REINO] Vigilancia en pausa... (próximo escaneo en 10 segundos)\n");
        sleep(30);
    }
}

int main() {
    // Configurar manejo de señales
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("🏰 ========================================== 🏰\n");
    printf("     SISTEMA DE VIGILANCIA DEL REINO USB\n");
    printf("        Detector de Infiltraciones\n");
    printf("🏰 ========================================== 🏰\n\n");
    
    printf("🛡️  Iniciando sistemas de seguridad...\n");
    printf("👁️  Configurando vigilancia de fronteras...\n");
    printf("📊 Umbral de alerta configurado en: %d%% de cambios\n\n", UMBRAL_CAMBIOS);
    
    // Iniciar monitoreo
    monitor_usb_devices();
    
    printf("\n🏰 [REINO] Vigilancia finalizada. Que la paz reine en el reino.\n");
    return 0;
}

//gcc -o usb-claude usb_claude.c -lssl -lcrypto