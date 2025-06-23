#!/bin/bash
# archivo: low_listen_ports.sh

# Puertos bajos: algunos sospechosos y otros esperados
# 21 (FTP, esperado), 22 (SSH, esperado), 23 (Telnet, sospechoso), 25 (SMTP, esperado), 111 (RPC, sospechoso)
PUERTOS=(21 22 23 25 111)

cleanup() {
    echo -e "\nCerrando puertos en escucha..."
    kill $(jobs -p) 2>/dev/null
    exit 0
}

trap cleanup SIGINT SIGTERM

echo "Abriendo puertos bajos en escucha: ${PUERTOS[*]}"
for puerto in "${PUERTOS[@]}"; do
    nc -lk -p "$puerto" >/dev/null 2>&1 &
    echo "Puerto $puerto escuchando..."
done

echo "Presiona Ctrl+C para cerrar todos los puertos."

while true; do
    sleep 1
done