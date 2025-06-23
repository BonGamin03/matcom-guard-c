#!/bin/bash

EXECUTABLE="../../Scanners/Ports/port_scanner"
#EXECUTABLE="../../GST"

# Colores para salida
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # Sin color

function print_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✔ $2${NC}"
    else
        echo -e "${RED}✘ $2${NC}"
    fi
}

echo "==== CASOS DE PRUEBA: ESCANEO DE PUERTOS ===="

# 1. Escaneo de un puerto abierto (por ejemplo, 80 en google.com)
$EXECUTABLE -p google.com 80 > out1.txt 2>&1
grep -qi "abierto" out1.txt
print_result $? "Puerto abierto (google.com:80)"

# 2. Escaneo de un puerto cerrado (por ejemplo, 81 en google.com)
$EXECUTABLE -p google.com 81 > out2.txt 2>&1
grep -qi "cerrado" out2.txt
print_result $? "Puerto cerrado (google.com:81)"

# 3. Escaneo de un rango de puertos (por ejemplo, 79-81 en google.com)
$EXECUTABLE -p google.com 79-81 > out3.txt 2>&1
grep -qi "79" out3.txt && grep -qi "80" out3.txt && grep -qi "81" out3.txt
print_result $? "Rango de puertos (google.com:79-81)"

# 4. Escaneo de puerto inválido (por ejemplo, -1)
$EXECUTABLE -p google.com -1 > out4.txt 2>&1
grep -qi "inválido\|error" out4.txt
print_result $? "Puerto inválido (google.com:-1)"

# 5. Escaneo de host inválido
$EXECUTABLE -p noexiste.abc 80 > out5.txt 2>&1
grep -qi "inválido\|error\|no encontrado" out5.txt
print_result $? "Host inválido (noexiste.abc:80)"

# Limpieza
rm out1.txt out2.txt out3.txt out4.txt out5.txt

echo "==== FIN DE LOS CASOS DE PRUEBA ===="