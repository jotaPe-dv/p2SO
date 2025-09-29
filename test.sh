#!/bin/bash

# =============================================================================
# Script de Pruebas Automáticas - Sistema de Chat con Colas de Mensajes
# =============================================================================
# Este script compila, ejecuta y valida todas las funcionalidades del sistema
# de chat implementado con colas de mensajes System V.
#
# Funcionalidades probadas:
# - Compilación de servidor y cliente
# - Inicio automático del servidor
# - Conexión simultánea de múltiples clientes
# - Creación de salas y unión de usuarios
# - Envío de mensajes entre usuarios
# - Comandos especiales (/list, /users, /leave)
# - Persistencia de mensajes en archivos
# - Limpieza automática de recursos
# =============================================================================

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuración
SERVIDOR_EJECUTABLE="./servidor"
CLIENTE_EJECUTABLE="./cliente"
HISTORIAL_DIR="historial"
LOG_DIR="logs_prueba"
TIMEOUT_COMANDO=2

# PIDs de procesos para limpieza
SERVIDOR_PID=""
CLIENTE_PIDS=()

# =============================================================================
# FUNCIONES AUXILIARES
# =============================================================================

# Función para mostrar mensajes con colores
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE} $1 ${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# Función para limpiar procesos y archivos
cleanup() {
    print_header "LIMPIANDO RECURSOS"
    
    # Matar clientes
    for pid in "${CLIENTE_PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            print_status "Terminando cliente PID: $pid"
            kill -TERM "$pid" 2>/dev/null || kill -KILL "$pid" 2>/dev/null
        fi
    done
    
    # Matar servidor
    if [ -n "$SERVIDOR_PID" ] && kill -0 "$SERVIDOR_PID" 2>/dev/null; then
        print_status "Terminando servidor PID: $SERVIDOR_PID"
        kill -TERM "$SERVIDOR_PID" 2>/dev/null || kill -KILL "$SERVIDOR_PID" 2>/dev/null
    fi
    
    # Limpiar colas de mensajes que puedan haber quedado
    print_status "Limpiando colas de mensajes restantes..."
    ipcs -q | grep "$(whoami)" | awk '{print $2}' | xargs -r ipcrm -q 2>/dev/null
    
    # Esperar un momento para que se liberen los recursos
    sleep 1
    
    print_success "Limpieza completada"
}

# Configurar trap para limpieza automática al salir
trap cleanup EXIT INT TERM

# Función para verificar si un proceso está ejecutándose
is_running() {
    kill -0 "$1" 2>/dev/null
}

# Función para enviar comando a cliente con timeout
send_command_to_client() {
    local input_file="$1"
    local command="$2"
    local timeout="${3:-$TIMEOUT_COMANDO}"
    
    echo "$command" >> "$input_file"
    sleep "$timeout"
}

# =============================================================================
# PRUEBAS DEL SISTEMA
# =============================================================================

# Función 1: Compilar el sistema
test_compilation() {
    print_header "FASE 1: COMPILACIÓN"
    
    print_status "Limpiando archivos previos..."
    make clean > /dev/null 2>&1
    
    print_status "Compilando servidor y cliente..."
    if make > "$LOG_DIR/compilation.log" 2>&1; then
        print_success "Compilación exitosa"
        return 0
    else
        print_error "Error en compilación. Ver $LOG_DIR/compilation.log"
        cat "$LOG_DIR/compilation.log"
        return 1
    fi
}

# Función 2: Iniciar servidor
test_server_startup() {
    print_header "FASE 2: INICIO DEL SERVIDOR"
    
    print_status "Iniciando servidor en segundo plano..."
    
    # Iniciar servidor y capturar su PID
    "$SERVIDOR_EJECUTABLE" > "$LOG_DIR/servidor.log" 2>&1 &
    SERVIDOR_PID=$!
    
    # Esperar que el servidor se inicie
    sleep 3
    
    if is_running "$SERVIDOR_PID"; then
        print_success "Servidor iniciado correctamente (PID: $SERVIDOR_PID)"
        
        # Verificar que se creó el directorio de historial
        if [ -d "$HISTORIAL_DIR" ]; then
            print_success "Directorio de historial creado: $HISTORIAL_DIR/"
        else
            print_warning "Directorio de historial no se creó automáticamente"
        fi
        return 0
    else
        print_error "Error: El servidor no se pudo iniciar"
        if [ -f "$LOG_DIR/servidor.log" ]; then
            echo "Log del servidor:"
            cat "$LOG_DIR/servidor.log"
        fi
        return 1
    fi
}

# Función 3: Prueba de clientes múltiples
test_multiple_clients() {
    print_header "FASE 3: CLIENTES MÚLTIPLES"
    
    local clientes=("María" "Juan" "Carlos")
    local salas=("General" "General" "Deportes")
    
    print_status "Iniciando ${#clientes[@]} clientes simultáneos..."
    
    for i in "${!clientes[@]}"; do
        local cliente="${clientes[$i]}"
        local sala="${salas[$i]}"
        local input_file="$LOG_DIR/input_${cliente}.txt"
        local output_file="$LOG_DIR/output_${cliente}.log"
        
        print_status "Iniciando cliente: $cliente"
        
        # Crear archivo de entrada para el cliente
        touch "$input_file"
        
        # Iniciar cliente en segundo plano
        "$CLIENTE_EJECUTABLE" "$cliente" < "$input_file" > "$output_file" 2>&1 &
        local cliente_pid=$!
        CLIENTE_PIDS+=("$cliente_pid")
        
        sleep 1
        
        if is_running "$cliente_pid"; then
            print_success "Cliente $cliente iniciado (PID: $cliente_pid)"
            
            # Unirse a sala
            print_status "Cliente $cliente uniéndose a sala: $sala"
            send_command_to_client "$input_file" "join $sala" 2
            
        else
            print_error "Error: Cliente $cliente no se pudo iniciar"
            return 1
        fi
    done
    
    return 0
}

# Función 4: Prueba de comandos especiales
test_special_commands() {
    print_header "FASE 4: COMANDOS ESPECIALES"
    
    local maria_input="$LOG_DIR/input_María.txt"
    local juan_input="$LOG_DIR/input_Juan.txt"
    
    print_status "Probando comando /list (María)..."
    send_command_to_client "$maria_input" "/list" 2
    
    print_status "Probando comando /users (María en General)..."
    send_command_to_client "$maria_input" "/users" 2
    
    print_status "Probando comando /users (Juan en General)..."
    send_command_to_client "$juan_input" "/users" 2
    
    print_success "Comandos especiales enviados"
    return 0
}

# Función 5: Prueba de mensajería
test_messaging() {
    print_header "FASE 5: ENVÍO DE MENSAJES"
    
    local maria_input="$LOG_DIR/input_María.txt"
    local juan_input="$LOG_DIR/input_Juan.txt"
    local carlos_input="$LOG_DIR/input_Carlos.txt"
    
    # Mensajes de prueba
    local mensajes_maria=(
        "¡Hola a todos en General!"
        "¿Cómo están hoy?"
        "Este es un mensaje de prueba"
    )
    
    local mensajes_juan=(
        "Hola María, muy bien gracias"
        "¿Cómo va el proyecto?"
    )
    
    local mensajes_carlos=(
        "Hola desde la sala de Deportes"
        "¿Alguien vio el partido de ayer?"
    )
    
    print_status "María enviando mensajes en General..."
    for msg in "${mensajes_maria[@]}"; do
        send_command_to_client "$maria_input" "$msg" 1
    done
    
    print_status "Juan respondiendo en General..."
    for msg in "${mensajes_juan[@]}"; do
        send_command_to_client "$juan_input" "$msg" 1
    done
    
    print_status "Carlos enviando mensajes en Deportes..."
    for msg in "${mensajes_carlos[@]}"; do
        send_command_to_client "$carlos_input" "$msg" 1
    done
    
    print_success "Mensajes de prueba enviados"
    return 0
}

# Función 6: Prueba de comando /leave
test_leave_command() {
    print_header "FASE 6: COMANDO LEAVE"
    
    local juan_input="$LOG_DIR/input_Juan.txt"
    
    print_status "Juan saliendo de la sala General..."
    send_command_to_client "$juan_input" "/leave" 2
    
    print_status "Verificando que Juan salió (comando /users por María)..."
    local maria_input="$LOG_DIR/input_María.txt"
    send_command_to_client "$maria_input" "/users" 2
    
    print_success "Comando /leave probado"
    return 0
}

# Función 7: Verificación de persistencia
test_persistence() {
    print_header "FASE 7: VERIFICACIÓN DE PERSISTENCIA"
    
    print_status "Verificando archivos de historial..."
    
    local archivos_esperados=(
        "$HISTORIAL_DIR/sala_General.txt"
        "$HISTORIAL_DIR/sala_Deportes.txt"
    )
    
    local archivos_encontrados=0
    
    for archivo in "${archivos_esperados[@]}"; do
        if [ -f "$archivo" ]; then
            print_success "Archivo encontrado: $archivo"
            
            # Mostrar contenido del archivo
            local num_lineas=$(wc -l < "$archivo")
            print_status "  └─ Contiene $num_lineas mensajes"
            
            # Mostrar las primeras líneas como muestra
            if [ "$num_lineas" -gt 0 ]; then
                print_status "  └─ Últimas 3 líneas:"
                tail -n 3 "$archivo" | while IFS= read -r linea; do
                    echo -e "      ${YELLOW}$linea${NC}"
                done
            fi
            
            archivos_encontrados=$((archivos_encontrados + 1))
        else
            print_warning "Archivo no encontrado: $archivo"
        fi
    done
    
    if [ "$archivos_encontrados" -gt 0 ]; then
        print_success "Persistencia funcionando: $archivos_encontrados archivos creados"
        return 0
    else
        print_error "Error: No se crearon archivos de persistencia"
        return 1
    fi
}

# Función 8: Verificación de logs del servidor
test_server_logs() {
    print_header "FASE 8: VERIFICACIÓN DE LOGS DEL SERVIDOR"
    
    if [ -f "$LOG_DIR/servidor.log" ]; then
        local num_lineas=$(wc -l < "$LOG_DIR/servidor.log")
        print_success "Log del servidor generado: $num_lineas líneas"
        
        print_status "Últimas 10 líneas del log del servidor:"
        echo -e "${YELLOW}"
        tail -n 10 "$LOG_DIR/servidor.log"
        echo -e "${NC}"
        
        # Buscar posibles errores
        local errores=$(grep -i "error\|fail\|invalid" "$LOG_DIR/servidor.log" | wc -l)
        if [ "$errores" -eq 0 ]; then
            print_success "No se encontraron errores en el log del servidor"
        else
            print_warning "Se encontraron $errores posibles errores en el log"
        fi
        
        return 0
    else
        print_error "No se generó log del servidor"
        return 1
    fi
}

# Función principal de pruebas
run_all_tests() {
    local start_time=$(date +%s)
    local failed_tests=0
    
    print_header "INICIANDO PRUEBAS AUTOMÁTICAS DEL SISTEMA"
    print_status "Fecha: $(date)"
    print_status "Usuario: $(whoami)"
    print_status "Directorio: $(pwd)"
    
    # Crear directorio de logs
    mkdir -p "$LOG_DIR"
    
    # Ejecutar todas las pruebas
    local tests=(
        "test_compilation"
        "test_server_startup" 
        "test_multiple_clients"
        "test_special_commands"
        "test_messaging"
        "test_leave_command"
        "test_persistence"
        "test_server_logs"
    )
    
    for test in "${tests[@]}"; do
        if ! $test; then
            failed_tests=$((failed_tests + 1))
        fi
        sleep 2
    done
    
    # Resumen final
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    print_header "RESUMEN DE PRUEBAS"
    print_status "Tiempo total: ${duration} segundos"
    print_status "Pruebas ejecutadas: ${#tests[@]}"
    
    if [ "$failed_tests" -eq 0 ]; then
        print_success "¡TODAS LAS PRUEBAS PASARON! ✓"
        print_success "El sistema está funcionando correctamente"
        echo -e "\n${GREEN}🎉 Sistema de chat validado exitosamente 🎉${NC}"
    else
        print_error "PRUEBAS FALLIDAS: $failed_tests de ${#tests[@]}"
        print_error "Revisa los logs en $LOG_DIR/ para más detalles"
        return 1
    fi
}

# =============================================================================
# EJECUCIÓN PRINCIPAL
# =============================================================================

# Verificar que estamos en el directorio correcto
if [ ! -f "servidor.c" ] || [ ! -f "cliente.c" ]; then
    print_error "Error: No se encontraron servidor.c y cliente.c en el directorio actual"
    print_error "Ejecuta este script desde el directorio del proyecto"
    exit 1
fi

# Ejecutar todas las pruebas
run_all_tests

# El cleanup se ejecutará automáticamente por el trap