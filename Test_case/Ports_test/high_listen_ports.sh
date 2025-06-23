#!/bin/bash
# archivo: high_listen_ports.sh

# Puertos altos: algunos riesgosos y otros esperados
# 31337 (Back Orifice, muy sospechoso), 6667 (IRC, sospechoso), 8080 (HTTP alternativo, esperado), 8443 (HTTPS alternativo, esperado), 2323 (Telnet alternativo, sospechoso)
PUERTOS=(31337 6667 8080 8443 2323)

cleanup() {
    echo -e "\nCerrando puertos en escucha..."
    kill $(jobs -p) 2>/dev/null
    exit 0
}

trap cleanup SIGINT SIGTERM

echo "Abriendo puertos altos en escucha: ${PUERTOS[*]}"
for puerto in "${PUERTOS[@]}"; do
    nc -lk -p "$puerto" >/dev/null 2>&1 &
    echo "Puerto $puerto escuchando..."
done

echo "Presiona Ctrl+C para cerrar todos los puertos."

while true; do
    sleep 1
done