#!/bin/bash

echo "==== Caso 1: Puerto legítimo (SSH 22) ===="
echo "Asegúrate de que el servicio SSH está activo:"
sudo systemctl start sshd
echo "SSH debería estar escuchando en el puerto 22."

echo
echo "==== Caso 2: Puerto sospechoso (31337) ===="
echo "Abriendo puerto 31337 con nc en segundo plano..."
nohup nc -l 31337 >/dev/null 2>&1 &
NC_PID=$!
echo "nc escuchando en 31337 (PID $NC_PID)"

echo
echo "==== Caso 3: Puerto alto inesperado (HTTP 4444) ===="
echo "Abriendo servidor HTTP en puerto 4444 con python3 en segundo plano..."
nohup python3 -m http.server 4444 >/dev/null 2>&1 &
PY_PID=$!
echo "python3 http.server escuchando en 4444 (PID $PY_PID)"

echo
echo "==== Caso 4: Timeout en puerto cerrado (9999) ===="
echo "No se abre ningún servicio en el puerto 9999 (debe estar cerrado)."

echo
echo "==== Estado actual de los puertos de prueba ===="
echo "Puedes ejecutar tu escáner de puertos ahora."
echo "Cuando termines, ejecuta este script con el argumento stop para cerrar los servicios de prueba:"
echo "    ./este_script.sh stop"

# Si el usuario pasa "stop" como argumento, mata los procesos de prueba
if [[ "$1" == "stop" ]]; then
    echo "Cerrando servicios de prueba..."
    sudo systemctl stop sshd
    pkill -f "nc -l 31337"
    pkill -f "python3 -m http.server 4444"
    echo "Todos los servicios de prueba han sido detenidos."
fi