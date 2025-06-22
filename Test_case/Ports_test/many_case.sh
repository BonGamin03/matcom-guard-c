#!/bin/bash

# Puertos a escuchar
PUERTOS=(1433 1521 1723 3306 3389 4444 5000 5432 5900 6000 6379 6666 6667 6969 8000 8069 8080 8081 9000 9001 9200 9300 10000 11211 12345 27017 27018 31337 54321)
# 20 21 23 25 53 80 110 143 161 389 443 445 465 514 587 993 995 

# Iniciar servidores de prueba en los puertos especificados
for PUERTO in "${PUERTOS[@]}"; do
    # Inicia un servidor en cada puerto en segundo plano
    while true; do
        echo "Servidor de prueba en puerto $PUERTO" | nc -l -p "$PUERTO"
    done &
    echo "Servidor escuchando en puerto $PUERTO"
done

echo "Servidores de prueba iniciados en los puertos: ${PUERTOS[*]}"
echo "Presiona Ctrl+C para detenerlos."

# Mantener el script corriendo
wait


