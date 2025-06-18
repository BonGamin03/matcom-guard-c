#!/bin/bash

# Script: ejecutar_c.sh
# Uso: ./ejecutar_c.sh archivo.c [argumentos]

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Verificar si se proporcionó un archivo
if [ $# -eq 0 ]; then
    echo -e "${RED}Error:${NC} No se especificó ningún archivo .c"
    echo -e "Uso: ${GREEN}./ejecutar_c.sh archivo.c${NC} [argumentos]"
    exit 1
fi

ARCHIVO_C="$1"
NOMBRE_EJECUTABLE="${ARCHIVO_C%.*}"  # Elimina la extensión .c

# Verificar si el archivo existe
if [ ! -f "$ARCHIVO_C" ]; then
    echo -e "${RED}Error:${NC} El archivo '$ARCHIVO_C' no existe"
    exit 1
fi

# Compilar
echo -e "${YELLOW}Compilando $ARCHIVO_C ...${NC}"
gcc -Wall -Wextra -std=c11 "$ARCHIVO_C" -o "$NOMBRE_EJECUTABLE"

# Verificar si la compilación fue exitosa
if [ $? -ne 0 ]; then
    echo -e "${RED}Error:${NC} Fallo en la compilación"
    exit 1
fi

# Ejecutar con argumentos adicionales (si los hay)
echo -e "${GREEN}Ejecutando ./$NOMBRE_EJECUTABLE ...${NC}"
echo -e "----------------------------------------"
shift  # Elimina el primer argumento (el archivo .c)
./"$NOMBRE_EJECUTABLE" "$@"

# Limpieza opcional (descomenta si quieres eliminar el ejecutable después)
# rm "$NOMBRE_EJECUTABLE"





#  ./Gran_Salon_app.sh interfaz/app.c

# Fin del script