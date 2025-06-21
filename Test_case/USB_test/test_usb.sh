Contenido pegado
10.27 KB •297 líneas
•
El formato puede ser inconsistente con la fuente
#!/bin/bash

# Generador de Amenazas de Prueba para MatCom Guard
# Simula diferentes tipos de actividades maliciosas en dispositivos USB

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Variables
TEST_DIR="/tmp/usb_test_mount"
MALWARE_SAMPLES_DIR="$TEST_DIR/malware_samples"
NORMAL_FILES_DIR="$TEST_DIR/normal_files"

print_banner() {
    echo -e "${RED}"
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo "║                     GENERADOR DE AMENAZAS                    ║"
    echo "║                  Simulador de Actividad Maliciosa            ║"
    echo "║                                                               ║"
    echo "║           ⚠️  SOLO PARA PRUEBAS DE MATCOM GUARD ⚠️            ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
}

create_test_environment() {
    echo -e "${BLUE}🏗️  Creando entorno de pruebas...${NC}"
    
    # Crear directorio de prueba que simule un USB montado
    mkdir -p "$TEST_DIR"
    mkdir -p "$MALWARE_SAMPLES_DIR"
    mkdir -p "$NORMAL_FILES_DIR"
    mkdir -p "$TEST_DIR/Documents"
    mkdir -p "$TEST_DIR/Pictures"
    mkdir -p "$TEST_DIR/System"
    
    echo -e "${GREEN}✅ Entorno creado en: $TEST_DIR${NC}"
}

generate_normal_files() {
    echo -e "${CYAN}📄 Generando archivos normales...${NC}"
    
    # Documentos normales
    cat > "$NORMAL_FILES_DIR/readme.txt" << 'EOF'
Este es un archivo de texto normal.
Contiene información sobre el contenido del dispositivo.
Creado para pruebas de MatCom Guard.
EOF

    cat > "$NORMAL_FILES_DIR/config.ini" << 'EOF'
[Settings]
Language=Spanish
Theme=Dark
AutoSave=true
EOF

    # Imagen simulada (texto que simula datos binarios)
    dd if=/dev/urandom of="$TEST_DIR/Pictures/photo1.jpg" bs=1024 count=50 2>/dev/null
    dd if=/dev/urandom of="$TEST_DIR/Pictures/photo2.jpg" bs=1024 count=75 2>/dev/null
    
    # Documento simulado
    cat > "$TEST_DIR/Documents/report.docx" << 'EOF'
Reporte Mensual - Simulación
Este archivo simula un documento Word normal.
Datos: ventas, estadísticas, información corporativa.
EOF

    echo -e "${GREEN}✅ Archivos normales generados${NC}"
}

generate_suspicious_files() {
    echo -e "${YELLOW}⚠️  Generando archivos sospechosos...${NC}"
    
    # 1. ARCHIVO ANORMALMENTE GRANDE (>100MB)
    echo -e "${MAGENTA}   Creando archivo sospechosamente grande...${NC}"
    dd if=/dev/zero of="$MALWARE_SAMPLES_DIR/video_codec.exe" bs=1M count=120 2>/dev/null
    
    # 2. ARCHIVOS DUPLICADOS (Replicación maliciosa)
    echo -e "${MAGENTA}   Creando archivos duplicados...${NC}"
    cat > "$MALWARE_SAMPLES_DIR/invoice.pdf" << 'EOF'
FACTURA IMPORTANTE
Por favor abrir inmediatamente.
Contenido: Datos financieros críticos.
EOF
    
    # Crear copias idénticas con nombres diferentes
    cp "$MALWARE_SAMPLES_DIR/invoice.pdf" "$MALWARE_SAMPLES_DIR/invoice_copy.pdf"
    cp "$MALWARE_SAMPLES_DIR/invoice.pdf" "$MALWARE_SAMPLES_DIR/urgent_invoice.pdf"
    cp "$MALWARE_SAMPLES_DIR/invoice.pdf" "$TEST_DIR/Documents/invoice_backup.pdf"
    
    # 3. ARCHIVOS CON NOMBRES SOSPECHOSOS
    echo -e "${MAGENTA}   Creando archivos con nombres maliciosos...${NC}"
    cat > "$MALWARE_SAMPLES_DIR/system32.exe" << 'EOF'
Este archivo simula un ejecutable malicioso.
Normalmente contendría código dañino.
SOLO PARA PRUEBAS.
EOF
    
    cat > "$MALWARE_SAMPLES_DIR/update.bat" << 'EOF'
@echo off
REM Simulación de script malicioso
REM Normalmente ejecutaría comandos peligrosos
echo "Simulando actividad maliciosa..."
EOF
    
    # 4. ARCHIVOS OCULTOS SOSPECHOSOS
    echo -e "${MAGENTA}   Creando archivos ocultos...${NC}"
    cat > "$TEST_DIR/.hidden_payload" << 'EOF'
Payload oculto simulado.
Este archivo estaría oculto en un ataque real.
EOF
    
    cat > "$TEST_DIR/System/.sys_config" << 'EOF'
Configuración de sistema falsa.
Podría contener configuraciones maliciosas.
EOF
    
    # 5. ARCHIVOS CON EXTENSIONES MÚLTIPLES
    echo -e "${MAGENTA}   Creando archivos con extensiones engañosas...${NC}"
    cat > "$MALWARE_SAMPLES_DIR/document.pdf.exe" << 'EOF'
Archivo que se hace pasar por PDF pero es ejecutable.
Técnica común de ingeniería social.
EOF
    
    cat > "$MALWARE_SAMPLES_DIR/photo.jpg.scr" << 'EOF'
Simulación de imagen que es realmente un screensaver malicioso.
EOF
    
    echo -e "${GREEN}✅ Archivos sospechosos generados${NC}"
}

simulate_real_time_threats() {
    echo -e "${RED}🚨 Simulando amenazas en tiempo real...${NC}"
    echo -e "${YELLOW}   (Ejecuta MatCom Guard en otra terminal para ver las detecciones)${NC}"
    
    sleep 2
    
    # Simular modificaciones en tiempo real
    echo -e "${MAGENTA}   Modificando archivos existentes...${NC}"
    echo "MODIFICACIÓN SOSPECHOSA - $(date)" >> "$NORMAL_FILES_DIR/readme.txt"
    
    sleep 3
    
    # Crear más duplicados
    echo -e "${MAGENTA}   Creando más archivos duplicados...${NC}"
    cp "$MALWARE_SAMPLES_DIR/invoice.pdf" "$TEST_DIR/invoice_$(date +%s).pdf"
    cp "$MALWARE_SAMPLES_DIR/invoice.pdf" "$TEST_DIR/final_invoice.pdf"
    
    sleep 2
    
    # Crear archivo muy grande
    echo -e "${MAGENTA}   Creando archivo masivo...${NC}"
    dd if=/dev/zero of="$TEST_DIR/temp_download.tmp" bs=1M count=200 2>/dev/null &
    
    sleep 3
    
    # Simular cambios de permisos
    echo -e "${MAGENTA}   Cambiando permisos sospechosamente...${NC}"
    chmod 777 "$MALWARE_SAMPLES_DIR/system32.exe" 2>/dev/null
    
    echo -e "${GREEN}✅ Simulación de amenazas completada${NC}"
}

generate_advanced_threats() {
    echo -e "${CYAN}🔬 Generando amenazas avanzadas...${NC}"
    
    # Crear estructura de directorio sospechosa
    mkdir -p "$TEST_DIR/System32/drivers"
    mkdir -p "$TEST_DIR/Windows/Temp"
    mkdir -p "$TEST_DIR/Program Files/Common Files"
    
    # Archivos que simulan infección de autorun
    cat > "$TEST_DIR/autorun.inf" << 'EOF'
[autorun]
open=malware.exe
icon=folder.ico
action=Open folder to view files
EOF
    
    cat > "$TEST_DIR/malware.exe" << 'EOF'
Simulación de malware de autorun.
En un caso real, se ejecutaría automáticamente.
EOF
    
    # Simular archivos de ransomware
    for i in {1..5}; do
        cat > "$TEST_DIR/Documents/document_$i.txt.encrypted" << 'EOF'
¡TUS ARCHIVOS HAN SIDO ENCRIPTADOS!
Este es un archivo simulado de ransomware.
Para recuperar tus archivos, sigue las instrucciones...
EOF
    done
    
    # Crear archivo README de ransomware
    cat > "$TEST_DIR/README_DECRYPT.txt" << 'EOF'
🔒 TODOS TUS ARCHIVOS HAN SIDO ENCRIPTADOS 🔒

Esta es una simulación de mensaje de ransomware.
En un ataque real, aquí estarían las instrucciones de pago.

SOLO PARA PRUEBAS DE MATCOM GUARD.
EOF
    
    echo -e "${GREEN}✅ Amenazas avanzadas generadas${NC}"
}

show_test_summary() {
    echo -e "${CYAN}"
    echo "╔═══════════════════════════════════════════════════════════════╗"
    echo "║                    RESUMEN DE AMENAZAS                       ║"
    echo "╚═══════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
    
    echo -e "${YELLOW}📍 Ubicación de pruebas: ${CYAN}$TEST_DIR${NC}"
    echo
    echo -e "${RED}🚨 Amenazas generadas:${NC}"
    echo -e "  ${MAGENTA}• Archivo muy grande:${NC} video_codec.exe (120MB)"
    echo -e "  ${MAGENTA}• Archivos duplicados:${NC} invoice.pdf (4 copias)"
    echo -e "  ${MAGENTA}• Ejecutables sospechosos:${NC} system32.exe, update.bat"
    echo -e "  ${MAGENTA}• Archivos ocultos:${NC} .hidden_payload, .sys_config"
    echo -e "  ${MAGENTA}• Extensiones engañosas:${NC} document.pdf.exe, photo.jpg.scr"
    echo -e "  ${MAGENTA}• Autorun malicioso:${NC} autorun.inf + malware.exe"
    echo -e "  ${MAGENTA}• Simulación ransomware:${NC} archivos .encrypted"
    echo
    echo -e "${GREEN}📋 Archivos normales:${NC}"
    echo -e "  ${CYAN}• Documentos:${NC} readme.txt, config.ini, report.docx"
    echo -e "  ${CYAN}• Imágenes:${NC} photo1.jpg, photo2.jpg"
    echo
    echo -e "${YELLOW}🔧 Para probar MatCom Guard:${NC}"
    echo -e "  ${GREEN}1.${NC} Ejecuta: ${CYAN}sudo ./matcom_guard${NC}"
    echo -e "  ${GREEN}2.${NC} En otra terminal: ${CYAN}sudo mount --bind $TEST_DIR /media/usb_test${NC}"
    echo -e "  ${GREEN}3.${NC} Observa las detecciones en tiempo real"
    echo -e "  ${GREEN}4.${NC} Para limpiar: ${CYAN}./$(basename $0) --clean${NC}"
    echo
}

clean_test_environment() {
    echo -e "${YELLOW}🧹 Limpiando entorno de pruebas...${NC}"
    
    # Unmount si está montado
    umount /media/usb_test 2>/dev/null
    
    # Eliminar archivos de prueba
    rm -rf "$TEST_DIR"
    
    echo -e "${GREEN}✅ Entorno limpio${NC}"
}

# Función principal
main() {
    case "${1:-}" in
        --clean)
            clean_test_environment
            exit 0
            ;;
        --help)
            echo "Uso: $0 [--clean|--help]"
            echo "  --clean  Limpia el entorno de pruebas"
            echo "  --help   Muestra esta ayuda"
            exit 0
            ;;
    esac
    
    print_banner
    
    echo -e "${BLUE}🎯 Generando amenazas de prueba para MatCom Guard...${NC}"
    echo
    
    create_test_environment
    generate_normal_files
    generate_suspicious_files
    generate_advanced_threats
    
    echo
    echo -e "${YELLOW}⏱️  Iniciando simulación en tiempo real en 5 segundos...${NC}"
    echo -e "${YELLOW}   (Inicia MatCom Guard ahora en otra terminal)${NC}"
    sleep 5
    
    simulate_real_time_threats
    
    echo
    show_test_summary
    
    echo
    echo -e "${GREEN}🎉 ¡Generación de amenazas completada!${NC}"
    echo -e "${CYAN}💡 Tip: Monta el directorio de prueba para que MatCom Guard lo detecte:${NC}"
    echo -e "${CYAN}   sudo mkdir -p /media/usb_test${NC}"
    echo -e "${CYAN}   sudo mount --bind $TEST_DIR /media/usb_test${NC}"
}

# Ejecutar
main "$@"
