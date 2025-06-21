#!/bin/bash

# --- Abrir puerto legítimo (SSH) ---
echo "Iniciando servicio SSH (puerto 22)..."
sudo systemctl start sshd

# --- Abrir puerto sospechoso (31337) con nc ---
echo "Abriendo puerto 31337 con nc..."
nohup nc -l 31337 >/dev/null 2>&1 &
NC31337_PID=$!

# --- Abrir puerto alto inesperado (4444) con python3 http.server ---
echo "Abriendo servidor HTTP en puerto 4444 con python3..."
nohup python3 -m http.server 4444 >/dev/null 2>&1 &
PY4444_PID=$!

# --- Puerto cerrado (9999): no se abre nada ---

echo
echo "==== Puertos de prueba abiertos ===="
echo " - SSH en 22 (servicio real)"
echo " - nc en 31337 (sospechoso)"
echo " - python3 http.server en 4444 (HTTP no estándar)"
echo " - 9999 (cerrado, no se abre nada)"
echo
echo "Puedes ejecutar tu escáner de puertos ahora."
echo
echo "Cuando termines, ejecuta este script con el argumento stop para cerrar los servicios de prueba:"
echo "    ./casos_prueba_puertos.sh stop"

# --- Detener servicios si se pasa el argumento stop ---
if [[ "$1" == "stop" ]]; then
    echo "Cerrando servicios de prueba..."
    sudo systemctl stop sshd
    pkill -f "nc -l 31337"
    pkill -f "python3 -m http.server 4444"
    echo "Todos los servicios de prueba han sido detenidos."
fi