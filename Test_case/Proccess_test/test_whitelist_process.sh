#!/bin/bash

# Test para verificar que procesos en lista blanca NO generen alertas
# Simula compilación intensiva con gcc/g++

echo "⚪ INICIANDO TEST: Proceso en Lista Blanca (gcc)"
echo "📋 Descripción: Simula compilación intensiva que usa mucha CPU"
echo "🎯 Resultado esperado: MatCom Guard NO debe generar alerta"
echo "✅ gcc está en la lista blanca del sistema"
echo ""

# Crear un archivo C temporal para compilar
cat > /tmp/test_compilation.c << 'EOF'
#include <stdio.h>
#include <math.h>

// Función que hace cálculos intensivos para que la compilación sea pesada
double intensive_calculation(int n) {
    double result = 0.0;
    for (int i = 1; i <= n; i++) {
        result += sin(i) * cos(i) * tan(i) * sqrt(i) * log(i);
        for (int j = 1; j <= 1000; j++) {
            result += (double)(i * j) / (i + j + 1);
        }
    }
    return result;
}

int main() {
    printf("Resultado: %f\n", intensive_calculation(10000));
    return 0;
}
EOF

echo "📝 Archivo de prueba creado: /tmp/test_compilation.c"
echo "🔄 Iniciando múltiples compilaciones intensivas..."
echo "📊 Monitorea MatCom Guard - NO debería generar alertas para gcc"
echo ""

# Ejecutar múltiples compilaciones en paralelo para generar carga
for i in {1..5}; do
    echo "🔨 Compilación $i/5 iniciada..."
    
    # Compilar con optimizaciones intensivas en segundo plano
    gcc -O3 -funroll-loops -ffast-math -march=native \
        -ftree-vectorize -fomit-frame-pointer \
        /tmp/test_compilation.c -o /tmp/test_program_$i -lm &
    
    # Esperar un poco entre cada compilación
    sleep 2
done

echo ""
echo "🔄 Esperando que terminen las compilaciones..."
wait

echo ""
echo "✅ Compilaciones completadas"
echo "🧪 Ahora ejecutando los programas compilados..."

# Ejecutar los programas compilados
for i in {1..5}; do
    if [ -f "/tmp/test_program_$i" ]; then
        echo "▶️  Ejecutando programa $i..."
        /tmp/test_program_$i &
    fi
done

echo ""
echo "⏱️  Ejecutando compilación adicional con make simulado..."

# Simular un proceso make intensivo
cat > /tmp/Makefile << 'EOF'
CC=gcc
CFLAGS=-O3 -funroll-loops -ffast-math -march=native
TARGETS=prog1 prog2 prog3 prog4 prog5

all: $(TARGETS)

prog%: /tmp/test_compilation.c
	$(CC) $(CFLAGS) $< -o /tmp/$@ -lm
	@sleep 1

clean:
	rm -f /tmp/prog*

.PHONY: all clean
EOF

echo "🔨 Ejecutando make..."
make -f /tmp/Makefile -j4

echo ""
echo "🧹 Limpiando archivos temporales..."
rm -f /tmp/test_compilation.c /tmp/test_program_* /tmp/prog* /tmp/Makefile

echo ""
echo "✅ TEST COMPLETADO"
echo "📊 Resultado esperado: MatCom Guard NO debería haber generado alertas"
echo "🔍 Verifica en la salida de MatCom Guard que gcc aparezca como 'Lista Blanca'"