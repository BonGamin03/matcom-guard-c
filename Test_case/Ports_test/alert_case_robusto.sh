#!/bin/bash
# archivo: alert_case_robusto.sh

# Verificar si nc está instalado
if ! command -v nc &>/dev/null; then
    echo -e "\e[31m[ERROR]\e[0m El comando 'nc' (netcat) no está instalado."
    exit 1
fi

# Puertos esperados (bajos y altos)
PUERTOS_ESPERADOS=(22 80 443 8080 8443)
# Puertos sospechosos (bajos y altos)
PUERTOS_SOSPECHOSOS=(23 111 2323 31337 6667 12345)

# Función para limpiar procesos nc al salir
cleanup() {
    echo -e "\n\e[33mCerrando todos los puertos abiertos por este script...\e[0m"
    kill $(jobs -p) 2>/dev/null
    sudo killall nc 2>/dev/null
    exit 0
}

trap cleanup SIGINT SIGTERM

echo -e "\e[34mAbriendo puertos esperados:\e[0m ${PUERTOS_ESPERADOS[*]}"
for puerto in "${PUERTOS_ESPERADOS[@]}"; do
    sudo nc -lk -p "$puerto" >/dev/null 2>&1 &
    echo -e "\e[32m[OK]\e[0m Puerto esperado $puerto escuchando..."
done

echo -e "\e[34mAbriendo puertos sospechosos:\e[0m ${PUERTOS_SOSPECHOSOS[*]}"
for puerto in "${PUERTOS_SOSPECHOSOS[@]}"; do
    sudo nc -lk -p "$puerto" >/dev/null 2>&1 &
    echo -e "\e[31m[ALERTA]\e[0m Puerto sospechoso $puerto escuchando..."
done

echo -e "\n\e[36mInicia tu interfaz GST en otra terminal y observa la terminal de alertas.\e[0m"
echo -e "\e[36mDeberías ver alertas para los puertos sospechosos y OK para los esperados.\e[0m"
echo -e "\e[33mPresiona Ctrl+C en esta terminal para cerrar todos los puertos y finalizar la prueba.\e[0m"

# Mantener el script vivo hasta que el usuario lo cierre
while true; do
    sleep 1
done