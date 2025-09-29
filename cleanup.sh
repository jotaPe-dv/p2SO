#!/bin/bash

# =============================================================================
# Script de Limpieza y Mantenimiento - Sistema de Chat
# =============================================================================
# Este script limpia recursos del sistema, procesos colgados y archivos temp
# =============================================================================

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[✓]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[!]${NC} $1"; }
print_error() { echo -e "${RED}[✗]${NC} $1"; }

echo "============================================="
echo "    LIMPIEZA DEL SISTEMA DE CHAT"
echo "============================================="

# 1. Matar procesos servidor y cliente
print_info "Terminando procesos del sistema de chat..."

# Buscar y terminar servidores
SERVER_PIDS=$(pgrep -f "./servidor" 2>/dev/null)
if [ -n "$SERVER_PIDS" ]; then
    echo "$SERVER_PIDS" | while read pid; do
        print_info "Terminando servidor PID: $pid"
        kill -TERM "$pid" 2>/dev/null || kill -KILL "$pid" 2>/dev/null
    done
    print_success "Procesos servidor terminados"
else
    print_info "No hay procesos servidor ejecutándose"
fi

# Buscar y terminar clientes
CLIENT_PIDS=$(pgrep -f "./cliente" 2>/dev/null)
if [ -n "$CLIENT_PIDS" ]; then
    echo "$CLIENT_PIDS" | while read pid; do
        print_info "Terminando cliente PID: $pid"
        kill -TERM "$pid" 2>/dev/null || kill -KILL "$pid" 2>/dev/null
    done
    print_success "Procesos cliente terminados"
else
    print_info "No hay procesos cliente ejecutándose"
fi

# 2. Limpiar colas de mensajes
print_info "Limpiando colas de mensajes System V..."
QUEUE_COUNT=0

# Obtener colas del usuario actual
USER_QUEUES=$(ipcs -q 2>/dev/null | grep "$(whoami)" | awk '{print $2}' 2>/dev/null)

if [ -n "$USER_QUEUES" ]; then
    echo "$USER_QUEUES" | while read qid; do
        if [ -n "$qid" ] && [ "$qid" != "msqid" ]; then
            print_info "Eliminando cola de mensajes: $qid"
            ipcrm -q "$qid" 2>/dev/null
            QUEUE_COUNT=$((QUEUE_COUNT + 1))
        fi
    done
    print_success "Colas de mensajes limpiadas"
else
    print_info "No hay colas de mensajes del usuario actual"
fi

# 3. Limpiar archivos compilados
print_info "Limpiando archivos compilados..."
rm -f servidor cliente
if [[ ! -f servidor && ! -f cliente ]]; then
    print_success "Ejecutables eliminados"
fi

# 4. Limpiar archivos de log y temporales
print_info "Limpiando archivos temporales..."

# Limpiar logs de pruebas
if [ -d "logs_prueba" ]; then
    rm -rf logs_prueba
    print_success "Directorio logs_prueba eliminado"
fi

# Limpiar archivos temporales
rm -f *.log *.tmp compilation_errors.log
print_success "Archivos temporales eliminados"

# 5. Opción para limpiar historial (con confirmación)
if [ -d "historial" ]; then
    echo ""
    print_warning "¿Deseas eliminar el historial de mensajes? (s/N)"
    read -r response
    if [[ "$response" =~ ^[Ss]$ ]]; then
        rm -rf historial
        print_success "Historial eliminado"
    else
        print_info "Historial conservado"
        
        # Mostrar estadísticas del historial
        if [ -d "historial" ]; then
            HIST_FILES=$(find historial -name "*.txt" 2>/dev/null | wc -l)
            if [ "$HIST_FILES" -gt 0 ]; then
                print_info "Archivos de historial: $HIST_FILES"
                find historial -name "*.txt" -exec basename {} \; | while read filename; do
                    echo "  - $filename"
                done
            fi
        fi
    fi
fi

# 6. Verificar estado final
print_info "Verificando estado final del sistema..."

# Verificar procesos
REMAINING_PROCS=$(pgrep -f "(servidor|cliente)" 2>/dev/null | wc -l)
if [ "$REMAINING_PROCS" -eq 0 ]; then
    print_success "No hay procesos del sistema ejecutándose"
else
    print_warning "$REMAINING_PROCS procesos aún ejecutándose"
fi

# Verificar colas
REMAINING_QUEUES=$(ipcs -q 2>/dev/null | grep "$(whoami)" | wc -l)
if [ "$REMAINING_QUEUES" -eq 0 ]; then
    print_success "No hay colas de mensajes restantes"
else
    print_warning "$REMAINING_QUEUES colas de mensajes restantes"
fi

# 7. Mostrar resumen
echo ""
print_info "RESUMEN DE LIMPIEZA:"
echo "  - Procesos terminados: ✓"
echo "  - Colas de mensajes limpiadas: ✓" 
echo "  - Ejecutables eliminados: ✓"
echo "  - Archivos temporales eliminados: ✓"

# 8. Mostrar comandos útiles
echo ""
print_info "COMANDOS ÚTILES:"
echo "  Compilar:           make"
echo "  Pruebas simples:    ./test_simple.sh"
echo "  Pruebas completas:  ./test.sh"
echo "  Ver colas:          ipcs -q"
echo "  Ver procesos:       ps aux | grep -E '(servidor|cliente)'"

print_success "¡Limpieza completada!"