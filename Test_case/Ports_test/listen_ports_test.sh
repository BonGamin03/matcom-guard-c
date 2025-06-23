#!/bin/bash
# archivo: listen_ports_test.sh

# Esperados por proceso esperado (si tienes el servicio real corriendo)
 PUERTOS_ESPERADOS=(22 80 443)

# Esperados por proceso inesperado (simulados con nc)
PUERTOS_ESPERADOS_INESPERADO=(22 80 443)

# Sospechosos (backdoors, IRC, etc)
PUERTOS_SOSPECHOSOS=(23 31337 6667 54321)

# Altos sin servicio conocido
PUERTOS_ALTOS_DESCONOCIDOS=(20000 54321)

cleanup() {
    echo -e "\nCerrando puertos en escucha..."
    kill $(jobs -p) 2>/dev/null
    sudo killall nc 2>/dev/null
    exit 0
}

trap cleanup SIGINT SIGTERM

echo -e "\e[34mAbriendo puertos esperados por proceso inesperado (nc):\e[0m ${PUERTOS_ESPERADOS_INESPERADO[*]}"
for puerto in "${PUERTOS_ESPERADOS_INESPERADO[@]}"; do
    sudo nc -lk -p "$puerto" >/dev/null 2>&1 &
    echo -e "\e[33m[TEST]\e[0m Puerto esperado $puerto abierto por proceso inesperado (nc)..."
done

echo -e "\e[34mAbriendo puertos sospechosos:\e[0m ${PUERTOS_SOSPECHOSOS[*]}"
for puerto in "${PUERTOS_SOSPECHOSOS[@]}"; do
    sudo nc -lk -p "$puerto" >/dev/null 2>&1 &
    echo -e "\e[31m[ALERTA]\e[0m Puerto sospechoso $puerto abierto..."
done

echo -e "\e[34mAbriendo puertos altos sin servicio conocido:\e[0m ${PUERTOS_ALTOS_DESCONOCIDOS[*]}"
for puerto in "${PUERTOS_ALTOS_DESCONOCIDOS[@]}"; do
    sudo nc -lk -p "$puerto" >/dev/null 2>&1 &
    echo -e "\e[31m[ALERTA]\e[0m Puerto alto desconocido $puerto abierto..."
done

echo -e "\n\e[36mInicia tu interfaz GST y observa la terminal de alertas.\e[0m"
echo -e "\e[36mDeberías ver alertas por:\e[0m"
echo -e " - Puertos sospechosos abiertos"
echo -e " - Puertos esperados abiertos por proceso inesperado"
echo -e " - Puertos altos desconocidos abiertos"
echo -e "\e[33mPresiona Ctrl+C para cerrar todos los puertos y finalizar la prueba.\e[0m"

while true; do
    sleep 1
done