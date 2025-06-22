#!/bin/bash

# Test para generar alto uso de CPU (Proceso Malicioso)
# Este script simula un proceso que consume mucha CPU
# Debe generar alerta en MatCom Guard

echo "🔥 INICIANDO TEST: Proceso con Alto Uso de CPU"
echo "📋 Descripción: Simula un proceso malicioso que consume >70% CPU"
echo "🎯 Resultado esperado: MatCom Guard debe generar ALERTA"
echo "⏱️  Duración: Ejecutar por 30-60 segundos, luego presionar Ctrl+C"
echo ""
echo "🚀 Iniciando bucle intensivo de CPU en 3 segundos..."
sleep 1
echo "3..."
sleep 1
echo "2..."
sleep 1
echo "1..."
echo ""
echo "💥 EJECUTANDO BUCLE INTENSIVO - Presiona Ctrl+C para detener"
echo "📊 Monitorea MatCom Guard para ver la alerta..."

# Bucle infinito que consume CPU intensivamente
while true; do
    # Operaciones matemáticas intensivas
    for i in {1..100000}; do
        result=$((i * i * i + i * i + i))
    done
    
    # Operaciones de cadenas intensivas
    for i in {1..1000}; do
        dummy="test_string_$i"
        dummy="${dummy}_modified_${i}_extra_processing"
    done
done