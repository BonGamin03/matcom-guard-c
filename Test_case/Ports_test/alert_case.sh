#!/bin/bash

# Espera un momento para asegurarse de que los puertos estén abiertos
#sleep 2
#sudo killall nc

echo "Abriendo puertos riesgosos para la prueba..."
sudo nc -l -p 31337 &
sudo nc -l -p 2323 &
sudo nc -l -p 6667 &

echo "Inicia tu interfaz GST en otra terminal y observa la terminal de alertas."
echo "Después de unos segundos, deberías ver alertas de puertos sospechosos."
echo "Cuando termines, ejecuta: sudo killall nc"

