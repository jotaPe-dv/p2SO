#!/bin/bash

# =============================================================================
# Script de Pruebas Simples - Solo Compilación y Estructura
# =============================================================================
# Este script hace pruebas básicas que no requieren colas de mensajes System V
# Útil para verificar que el código está bien estructurado y compila correctamente
# =============================================================================

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

print_success() { echo -e "${GREEN}[✓]${NC} $1"; }
print_error() { echo -e "${RED}[✗]${NC} $1"; }
print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }

echo "============================================="
echo "  PRUEBAS BÁSICAS DEL SISTEMA DE CHAT"
echo "============================================="

# 1. Verificar archivos fuente
print_info "Verificando archivos fuente..."
if [[ -f "servidor.c" && -f "cliente.c" ]]; then
    print_success "Archivos fuente encontrados"
else
    print_error "Faltan archivos servidor.c o cliente.c"
    exit 1
fi

# 2. Verificar Makefile
if [ -f "Makefile" ]; then
    print_success "Makefile encontrado"
else
    print_error "Makefile no encontrado"
fi

# 3. Limpiar compilaciones previas
print_info "Limpiando archivos previos..."
make clean > /dev/null 2>&1

# 4. Compilar servidor
print_info "Compilando servidor..."
if gcc -Wall -Wextra -std=c99 -o servidor servidor.c 2>compilation_errors.log; then
    print_success "Servidor compilado exitosamente"
else
    print_error "Error compilando servidor:"
    cat compilation_errors.log
    rm -f compilation_errors.log
    exit 1
fi

# 5. Compilar cliente
print_info "Compilando cliente..."
if gcc -Wall -Wextra -std=c99 -pthread -o cliente cliente.c 2>>compilation_errors.log; then
    print_success "Cliente compilado exitosamente"
else
    print_error "Error compilando cliente:"
    cat compilation_errors.log
    rm -f compilation_errors.log
    exit 1
fi

# 6. Verificar ejecutables
if [[ -x "./servidor" && -x "./cliente" ]]; then
    print_success "Ejecutables creados correctamente"
else
    print_error "Error: Ejecutables no se crearon"
    exit 1
fi

# 7. Analizar funciones implementadas
print_info "Analizando funciones implementadas en servidor.c..."

# Buscar funciones clave
funciones_servidor=(
    "crear_sala"
    "buscar_sala"
    "agregar_usuario_a_sala"
    "remover_usuario_de_sala"
    "generar_lista_salas"
    "generar_lista_usuarios"
    "guardar_mensaje_en_archivo"
    "cargar_historial_sala"
    "crear_directorio_historial"
)

for func in "${funciones_servidor[@]}"; do
    if grep -q "^.*$func.*(" servidor.c; then
        print_success "Función encontrada: $func"
    else
        print_error "Función faltante: $func"
    fi
done

# 8. Verificar tipos de mensajes
print_info "Verificando tipos de mensajes definidos..."
tipos_mensaje=(
    "MTYPE_JOIN"
    "MTYPE_RESPONSE" 
    "MTYPE_MSG"
    "MTYPE_LIST"
    "MTYPE_USERS"
    "MTYPE_LEAVE"
)

for tipo in "${tipos_mensaje[@]}"; do
    if grep -q "#define.*$tipo" servidor.c; then
        print_success "Tipo de mensaje definido: $tipo"
    else
        print_error "Tipo de mensaje faltante: $tipo"
    fi
done

# 9. Verificar headers incluidos
print_info "Verificando headers incluidos..."
headers_requeridos=(
    "stdio.h"
    "stdlib.h"
    "string.h"
    "sys/ipc.h"
    "sys/msg.h"
    "time.h"
    "sys/stat.h"
)

for header in "${headers_requeridos[@]}"; do
    if grep -q "#include.*$header" servidor.c; then
        print_success "Header incluido: $header"
    else
        print_error "Header faltante: $header"
    fi
done

# 10. Verificar estructura de archivos
print_info "Verificando estructura del proyecto..."
if [ -f "README.md" ]; then
    print_success "README.md presente"
else
    print_error "README.md faltante"
fi

# 11. Mostrar estadísticas del código
print_info "Estadísticas del código:"
echo "  Líneas en servidor.c: $(wc -l < servidor.c)"
echo "  Líneas en cliente.c: $(wc -l < cliente.c)"
echo "  Funciones en servidor.c: $(grep -c "^[a-zA-Z_][a-zA-Z0-9_]*.*(" servidor.c)"
echo "  Funciones en cliente.c: $(grep -c "^[a-zA-Z_][a-zA-Z0-9_]*.*(" cliente.c)"

# 12. Crear directorio de historial de prueba
print_info "Creando directorio de historial de prueba..."
mkdir -p historial_test
if [ -d "historial_test" ]; then
    print_success "Directorio de prueba creado"
    rmdir historial_test
else
    print_error "No se pudo crear directorio de prueba"
fi

# Limpiar archivos temporales
rm -f compilation_errors.log

echo ""
print_success "¡TODAS LAS PRUEBAS BÁSICAS COMPLETADAS!"
echo ""
echo "Para pruebas completas con colas de mensajes, ejecuta:"
echo "  ./test.sh"
echo ""
echo "Para probar manualmente:"
echo "  1. ./servidor"
echo "  2. En otra terminal: ./cliente <nombre>"