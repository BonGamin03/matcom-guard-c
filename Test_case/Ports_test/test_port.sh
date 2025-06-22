#!/bin/bash

# Script de pruebas para el módulo de escaneo de puertos (RF3)
# Basado en los casos de prueba del documento MatCom Guard

# Función para mostrar un separador con mensaje
separator() {
    echo -e "\n\033[1;36m==========================================="
    echo -e " $1 "
    echo -e "===========================================\033[0m\n"
}

# Función para iniciar servicio en puerto
start_service() {
    port=$1
    service=$2
    echo "Iniciando servicio $service en puerto $port..."
    
    case $service in
        "SSH")
            sudo systemctl start ssh >/dev/null 2>&1
            ;;
        "NC")
            nc -l $port >/dev/null 2>&1 &
            NC_PID=$!
            ;;
        "HTTP")
            python3 -m http.server $port >/dev/null 2>&1 &
            HTTP_PID=$!
            ;;
    esac
    
    sleep 2
}

# Función para detener servicio
stop_service() {
    service=$1
    echo "Deteniendo servicio $service..."
    
    case $service in
        "SSH")
            sudo systemctl stop ssh >/dev/null 2>&1
            ;;
        "NC")
            kill $NC_PID >/dev/null 2>&1
            ;;
        "HTTP")
            kill $HTTP_PID >/dev/null 2>&1
            ;;
    esac
    
    sleep 2
}

# Función para ejecutar el escáner de puertos
run_port_scanner() {
    start=$1
    end=$2
    separator "EJECUTANDO ESCANEO DE PUERTOS $start-$end"
    ./Scanners/Ports/port_scanner 127.0.0.1 $start $end
}

# Función para verificar alertas
check_alert() {
    message=$1
    echo "Verificando alerta: $message"
    
    if grep -q "$message" Report/alertas_guard.txt; then
        echo -e "\033[1;32m✓ Alerta detectada correctamente\033[0m"
    else
        echo -e "\033[1;31m✗ Alerta NO detectada\033[0m"
    fi
}

# Limpiar alertas anteriores
> Report/alertas_guard.txt

# Caso 1: Puerto legítimo (SSH en puerto 22)
separator "CASO 1: PUERTO LEGÍTIMO (SSH EN PUERTO 22)"
start_service 22 "SSH"
run_port_scanner 1 1024
check_alert "✅ [OK] Puerto 22/tcp (SSH)"
stop_service "SSH"

# Caso 2: Puerto sospechoso (Netcat en puerto 31337)
separator "CASO 2: PUERTO SOSPECHOSO (31337)"
start_service 31337 "NC"
run_port_scanner 30000 32000
check_alert "🚨 [ALERTA] Puerto 31337/tcp abierto"
stop_service "NC"

# Caso 3: Puerto alto inesperado (HTTP en 4444)
separator "CASO 3: PUERTO ALTO INESPERADO (4444)"
start_service 4444 "HTTP"
run_port_scanner 4000 5000
check_alert "🚨 [ALERTA] Puerto 4444/tcp abierto"
stop_service "HTTP"

# Caso 4: Puerto cerrado (9999)
separator "CASO 4: PUERTO CERRADO (9999)"
run_port_scanner 9990 10000
check_alert "Puerto 9999/tcp cerrado"

# Caso 5: Cambio dinámico de puertos
separator "CASO 5: CAMBIO DINÁMICO DE PUERTOS"
echo "Iniciando servicio HTTP en puerto 8080"
start_service 8080 "HTTP"
run_port_scanner 8000 9000
check_alert "✅ [OK] Puerto 8080/tcp (HTTP alternativo)"

echo -e "\nDeteniendo servicio HTTP en 8080 e iniciando en 8888"
stop_service "HTTP"
start_service 8888 "HTTP"
run_port_scanner 8000 9000
check_alert "✅ [OK] Puerto 8888/tcp (HTTP alternativo)"
check_alert "🚨 [ALERTA] Puerto 8080/tcp"
stop_service "HTTP"

# Mostrar resumen final
separator "RESUMEN DE PRUEBAS"
echo "Alertas registradas:"
cat Report/alertas_guard.txt