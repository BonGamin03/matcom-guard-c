#!/bin/bash

# Test para verificar limpieza de procesos fantasma
# Crea procesos, los mata y verifica que MatCom Guard los detecte como inactivos

echo "👻 INICIANDO TEST: Limpieza de Procesos Fantasma"
echo "📋 Descripción: Crea procesos, los mata y verifica limpieza en MatCom Guard"
echo "🎯 Resultado esperado: Procesos eliminados no deben aparecer como activos"
echo ""

# Array para almacenar PIDs de procesos de prueba
declare -a test_pids=()

# Función para crear un proceso de prueba
create_test_process() {
    local name=$1
    local cpu_intensive=$2
    
    if [ "$cpu_intensive" = "true" ]; then
        # Proceso con uso intensivo de CPU
        bash -c "
            echo 'Proceso $name iniciado - PID: $$'
            while true; do
                for i in {1..10000}; do
                    result=\$((i * i))
                done
            done
        " &
    else
        # Proceso simple que solo espera
        bash -c "
            echo 'Proceso $name iniciado - PID: $$'
            while true; do
                sleep 1
            done
        " &
    fi
    
    local pid=$!
    test_pids+=($pid)
    echo "✅ Proceso '$name' creado con PID: $pid"
    return $pid
}

# Función para mostrar estado de procesos
show_process_status() {
    echo ""
    echo "📊 Estado actual de procesos de prueba:"
    echo "PID      Estado    Comando"
    echo "-------- --------- ----------------------------------------"
    
    for pid in "${test_pids[@]}"; do
        if kill -0 $pid 2>/dev/null; then
            cmd=$(ps -p $pid -o comm= 2>/dev/null || echo "N/A")
            echo "$pid    ACTIVO    $cmd"
        else
            echo "$pid    MUERTO    (proceso terminado)"
        fi
    done
    echo ""
}

echo "🚀 Creando procesos de prueba..."
echo ""

# Crear varios procesos de prueba
create_test_process "test_normal" false
sleep 1
create_test_process "test_intensive" true
sleep 1
create_test_process "test_daemon" false
sleep 1

echo ""
echo "📊 Procesos de prueba creados:"
show_process_status

echo "⏱️  Esperando 10 segundos para que MatCom Guard los detecte..."
sleep 10

echo "🔍 Verifica en MatCom Guard que estos procesos aparezcan en el reporte"
echo "📝 PIDs a buscar: ${test_pids[*]}"
echo ""
read -p "Presiona ENTER cuando hayas visto los procesos en MatCom Guard..."

echo ""
echo "💀 MATANDO PROCESOS DE PRUEBA..."
echo ""

# Matar procesos uno por uno
for i in "${!test_pids[@]}"; do
    pid=${test_pids[$i]}
    echo "🗡️  Matando proceso PID: $pid"
    
    if kill -0 $pid 2>/dev/null; then
        kill -TERM $pid 2>/dev/null
        sleep 2
        
        # Si sigue vivo, usar SIGKILL
        if kill -0 $pid 2>/dev/null; then
            echo "   ⚔️  Usando SIGKILL para PID: $pid"
            kill -KILL $pid 2>/dev/null
        fi
        
        # Verificar que murió
        sleep 1
        if kill -0 $pid 2>/dev/null; then
            echo "   ❌ ERROR: Proceso $pid sigue vivo"
        else
            echo "   ✅ Proceso $pid terminado exitosamente"
        fi
    else
        echo "   ⚠️  Proceso $pid ya estaba muerto"
    fi
    
    echo "   ⏱️  Esperando 5 segundos antes del siguiente..."
    sleep 5
done

echo ""
echo "📊 Estado final de procesos:"
show_process_status

echo "🔍 INSTRUCCIONES PARA VERIFICAR EL TEST:"
echo ""
echo "1. Observa la salida de MatCom Guard en las próximas iteraciones"
echo "2. Los PIDs ${test_pids[*]} NO deberían aparecer más en el reporte"
echo "3. Si aparecen, significa que hay un problema en la limpieza de procesos"
echo "4. El contador de 'procesos activos' debería haber disminuido"
echo ""
echo "⏱️  Esperando 30 segundos para que MatCom Guard actualice..."
sleep 30

echo ""
echo "✅ TEST COMPLETADO"
echo "📋 Resultado esperado:"
echo "   - Los procesos con PIDs ${test_pids[*]} NO deben aparecer en el reporte"
echo "   - El total de procesos activos debe haber disminuido"
echo "   - No debe haber alertas para procesos inexistentes"
echo ""
echo "🔍 Verifica estos puntos en la salida de MatCom Guard"