#!/bin/bash

# Colores para el menú
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
ORANGE='\033[1;91m'
NC='\033[0m' # No Color

# Instalar dependencias si no existen
check_dependencies() {
    if ! command -v nmap &> /dev/null; then
        echo -e "${YELLOW}[!] Instalando nmap...${NC}"
        sudo apt install -y nmap
    fi
   
    if ! command -v enscript &> /dev/null; then
        echo -e "${YELLOW}[!] Instalando enscript...${NC}"
        sudo apt install -y enscript
    fi
}

# Escaneo de archivos con SUID (peligrosos)
scan_files() {
    echo -e "${GREEN}[+] Escaneando archivos SUID... 🔄${NC}"


  # verificar si el archivo existe
    if [ -f "$HOME/Desktop/Matcom-Guard-C/Scanners/nombre_de_su_escaner" ]; then
        echo "El archivo "nombre_de_su_escaner" existe, procediendo con el escaneo ."
    else
        echo -e "${RED}[!] No se encontró "nombre_de_su_escaner"  en Scanners.${NC}"
        return 1
    fi
    # Compilar el escáner si no existe
        if [ ! -f "$HOME/Desktop/Matcom-Guard-C/Scanners/ nombre_de_su_escaner" ]; then
            echo "Compilando el escáner de puertos..."
            mkdir -p "$HOME/Desktop/Matcom-Guard-C/Scanners"
            cp nombre_de_su_escaner.c "$HOME/Desktop/Matcom-Guard-C/Scanners/"

            # Compilar el código C del escáner de sistema de archivos
            gcc -o "$HOME/Desktop/Matcom-Guard-C/Scanners/nombre_de_su_escaner" "$HOME/Desktop/Matcom-Guard-C/Scanners/nombre_de_su_escaner.c" -lpthread 2>/dev/null
            if [ $? -ne 0 ]; then
                echo -e "${RED}[!] Error al compilar el escáner.${NC}"
                return 1
            fi
        fi

        # Ejecutar el escáner de sistema de archivos
        if [ -f "$HOME/Desktop/Matcom-Guard-C/Scanners/nombre_de_su_escaner" ]; then
            # Pasar los argumentos al ejecutable
            echo ""
            echo -e "${YELLOW}------------------------------------------------------------------------${NC}"
            "$HOME/Desktop/Matcom-Guard-C/Scanners/nombre_de_su_escaner" | tee Report/reporte_sistema_archivos.txt
        else
            echo "[!] El ejecutable nombre_de_su_escaner no se encontró en ~/Scanners." | tee Report/reporte_sistema_archivos.txt
        fi

    echo -e "${YELLOW}------------------------------------------------------------------------${NC}"
    echo ""
    echo -e "${YELLOW}[+] Escaneo de sistema de archivos completado.${NC}"
    echo "🔍 Reporte guardado en: 📄Report/reporte_sistema_archivos.txt"
    # sudo find / -type f -perm /4000 2>/dev/null > Report/reporte_sistema_archivos.txt
    echo ""
}

# Escaneo de procesos y hilos 
scan_process() {
    echo -e "${GREEN}[+] Escaneando procesos y hilos... 🔄${NC}"
    

    # verificar si el archivo existe
    if [ -f "$HOME/Desktop/Matcom-Guard-C/Scanners/nombre_de_su_escaner" ]; then
        echo "El archivo "nombre_de_su_escaner" existe, procediendo con el escaneo ."
    else
        echo -e "${RED}[!] No se encontró "nombre_de_su_escaner"  en Scanners.${NC}"
        return 1
    fi
    # Compilar el escáner si no existe
        if [ ! -f "$HOME/Desktop/Matcom-Guard-C/Scanners/ nombre_de_su_escaner" ]; then
            echo "Compilando el escáner de puertos..."
            mkdir -p "$HOME/Desktop/Matcom-Guard-C/Scanners"
            cp nombre_de_su_escaner.c "$HOME/Desktop/Matcom-Guard-C/Scanners/"

            # Compilar el código C del escáner de procesos
            gcc -o "$HOME/Desktop/Matcom-Guard-C/Scanners/nombre_de_su_escaner" "$HOME/Desktop/Matcom-Guard-C/Scanners/nombre_de_su_escaner.c" -lpthread 2>/dev/null
            if [ $? -ne 0 ]; then
                echo -e "${RED}[!] Error al compilar el escáner.${NC}"
                return 1
            fi
        fi

        # Ejecutar el escáner de procesos
        if [ -f "$HOME/Desktop/Matcom-Guard-C/Scanners/nombre_de_su_escaner" ]; then
            # Pasar los argumentos al ejecutable
            echo ""
            echo -e "${YELLOW}------------------------------------------------------------------------${NC}"
            "$HOME/Desktop/Matcom-Guard-C/Scanners/nombre_de_su_escaner" | tee Report/reporte_procesos.txt
        else
            echo "[!] El ejecutable nombre_de_su_escaner no se encontró en ~/Scanners." | tee Report/reporte_procesos.txt
        fi
    echo -e "${YELLOW}------------------------------------------------------------------------${NC}"
    echo ""
    #ps aux | grep -v "grep" | awk '{print $1, $2, $3, $4, $11}' > Report/reporte_procesos.txt
    echo -e "${YELLOW}[+] Escaneo de procesos completado.${NC}"
    echo "🔍 Reporte guardado en: 📄Report/reporte_procesos.txt"
    #ps aux > Report/reporte_procesos.txt
    echo ""
}

# Escaneo de puertos (usando nmap o netstat)
scan_ports() {
    echo -e "${GREEN}[+] Escaneando puertos... 🔄${NC}"
    if [ -f "$HOME/Desktop/Matcom-Guard-C/Scanners/port_scanner.c" ]; then

        read -p "Ingrese la IP a escanear (dejar vacío para localhost): " ip
        if [ -z "$ip" ]; then
            ip="127.0.0.1"
        fi

        read -p "Ingrese el rango de puertos (ej: 1-1024): " port_range

        # Validar formato de rango de puertos
        if [[ ! "$port_range" =~ ^[0-9]+-[0-9]+$ ]]; then
            echo "Formato de rango de puertos inválido. Use: inicio-fin (ej: 1-1024)"
            return 1
        fi

        IFS="-" read start_port end_port <<< "$port_range"
        if [ "$start_port" -lt 1 ] || [ "$end_port" -gt 65535 ] || [ "$start_port" -gt "$end_port" ]; then
            echo "Rango de puertos inválido. Debe estar entre 1-65535 y inicio <= fin"
            return 1
        fi

        # Compilar el escáner de puertos si no existe
        if [ ! -f "$HOME/Desktop/Matcom-Guard-C/Scanners/port_scanner" ]; then
            echo "Compilando el escáner de puertos..."
            mkdir -p "$HOME/Desktop/Matcom-Guard-C/Scanners"
            cp port_scanner.c "$HOME/Desktop/Matcom-Guard-C/Scanners/"

            # Compilar el código C del escáner de puertos
            gcc -o "$HOME/Desktop/Matcom-Guard-C/Scanners/port_scanner" "$HOME/Desktop/Matcom-Guard-C/Scanners/port_scanner.c" -lpthread 2>/dev/null
            if [ $? -ne 0 ]; then
            echo -e "${RED}[!] Error al compilar el escáner de puertos.${NC}"
            return 1
            fi
        fi

        # Ejecutar el escáner de puertos y mostrar la salida en la terminal y en el archivo
        if [ -f "$HOME/Desktop/Matcom-Guard-C/Scanners/port_scanner" ]; then
            # Pasar los argumentos al ejecutable
            echo ""
            echo -e "${YELLOW}------------------------------------------------------------------------${NC}"
            "$HOME/Desktop/Matcom-Guard-C/Scanners/port_scanner" "$ip" "$start_port" "$end_port" | tee Report/reporte_puertos.txt
        else
            echo "[!] El ejecutable port_scanner no se encontró en ~/Scanners." | tee Report/reporte_puertos.txt
        fi
        echo -e "${YELLOW}------------------------------------------------------------------------${NC}"
        echo ""
        echo -e "${YELLOW}[+] Escaneo de puertos completado.${NC}"
        echo "🔍 Reporte guardado en: 📄Report/reporte_puertos.txt"
    else
        echo -e "${RED}[!] No se encontró port_scanner.c en Scanners.${NC}"
    fi
    echo ""
}

# Generar PDF unificado
generate_pdf() {
    echo -e "${GREEN}[+] Generando PDF...${NC}"
    enscript --header="Gran Salón del Trono|Reporte de Seguridad|Página $%" -p Report/reporte.ps Report/reporte_*.txt 2>/dev/null
    ps2pdf Report/reporte.ps Report/reporte_seguridad.pdf
    rm Report/reporte.ps 2>/dev/null
    echo -e "${YELLOW}[+] PDF generado: Report/reporte_seguridad.pdf${NC}"
}

# Función para abrir el PDF
open_pdf() {
    if [ -f "Report/reporte_seguridad.pdf" ]; then
        echo -e "${GREEN}[+] Abriendo el PDF...${NC}"
        
        # Detecta el visor de PDF predeterminado
        if command -v xdg-open &> /dev/null; then
            xdg-open Report/reporte_seguridad.pdf &
        elif command -v evince &> /dev/null; then
            evince Report/reporte_seguridad.pdf &
        elif command -v okular &> /dev/null; then
            okular Report/reporte_seguridad.pdf &
        else
            echo -e "${RED}[!] No se encontró un visor de PDF instalado.${NC}"
            echo "El PDF está disponible en: $(pwd)/Report/reporte_seguridad.pdf"
        fi
    else
        echo -e "${RED}[!] Error: El PDF no existe.${NC}"
        echo -e "Genera primero el reporte con la opción 5."
    fi
}


# Menú principal
show_menu() {
    clear
    echo -e "${ORANGE}========================================"
    echo "=== 🛡️ Gran Salón del Trono 🛡️ ==="
    echo -e "${NC}"
    echo "1. Escanear sistema de archivos (USB)"
    echo "2. Escanear Memoria"
    echo "3. Escanear puertos"
    echo "4. Escanear TODO"
    echo "5. Exportar reporte a PDF"
    echo "6. Abrir PDF generado"
    echo "7. Salir"
    echo
    echo -e "${ORANGE}========================================${NC}"
}

# Main
check_dependencies

while true; do
    show_menu
    read -p "Selecciona una opción: " opcion

    case $opcion in
        1) scan_files ;;
        2) scan_process ;;
        3) scan_ports ;;
        4) 
            scan_files
            scan_process
            scan_ports
            ;;
        5) generate_pdf ;;
        6) open_pdf ;;
    7) 
        echo -e "${YELLOW}[+] Saliendo...${NC}"
        exit 0
        ;;
    *) 
        echo -e "${RED}[!] Opción inválida${NC}"
        sleep 1
        ;;
    esac
    read -p "Presiona Enter para continuar..."
done

#./Gran_Salon.sh

# Fin del script

