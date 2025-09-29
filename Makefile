CC = gcc
CFLAGS = -Wall -Wextra -std=c99
PTHREAD_FLAGS = -pthread

all: servidor cliente

servidor: servidor.c
	$(CC) $(CFLAGS) -o servidor servidor.c

cliente: cliente.c
	$(CC) $(CFLAGS) $(PTHREAD_FLAGS) -o cliente cliente.c

clean:
	rm -f servidor cliente
	rm -rf historial logs_prueba
	rm -f *.log *.tmp

# Pruebas básicas (solo compilación y estructura)
test-simple: all
	@echo "Ejecutando pruebas básicas..."
	./test_simple.sh

# Pruebas completas (requiere colas de mensajes System V)
test-full: all
	@echo "Ejecutando pruebas completas..."
	./test.sh

# Limpieza completa del sistema
cleanup:
	@echo "Ejecutando limpieza del sistema..."
	./cleanup.sh

# Compilar con información de debug
debug: CFLAGS += -g -DDEBUG
debug: all

# Mostrar ayuda
help:
	@echo "=============================================="
	@echo "  Sistema de Chat con Colas de Mensajes"
	@echo "=============================================="
	@echo ""
	@echo "📋 TARGETS DISPONIBLES:"
	@echo "  all         - Compilar servidor y cliente"
	@echo "  clean       - Limpiar archivos compilados"
	@echo "  test-simple - Ejecutar pruebas básicas"
	@echo "  test-full   - Ejecutar pruebas completas"
	@echo "  cleanup     - Limpieza completa del sistema"
	@echo "  debug       - Compilar con información de debug"
	@echo "  help        - Mostrar esta ayuda"
	@echo ""
	@echo "🚀 USO RÁPIDO:"
	@echo "  make && ./servidor"
	@echo "  # En otra terminal:"
	@echo "  ./cliente <nombre>"
	@echo ""
	@echo "🧪 PRUEBAS:"
	@echo "  make test-simple  # Siempre funciona"
	@echo "  make test-full    # Requiere System V IPC"
	@echo ""
	@echo "📚 DOCUMENTACIÓN:"
	@echo "  README.md     - Documentación principal"
	@echo "  TECHNICAL.md  - Documentación técnica"
	@echo "  TESTING.md    - Guía de pruebas"

.PHONY: all clean test-simple test-full cleanup debug help