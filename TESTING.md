# Guía de Pruebas - Sistema de Chat con Colas de Mensajes

## 📋 Scripts de Pruebas Disponibles

### 1. **`test_simple.sh`** - Pruebas Básicas
**Propósito**: Verificar compilación, estructura de código y funciones implementadas
**Requisitos**: Solo GCC, no necesita colas de mensajes System V
**Tiempo**: ~10 segundos

```bash
./test_simple.sh
```

**Qué verifica:**
- ✅ Compilación exitosa de servidor y cliente
- ✅ Presencia de todas las funciones requeridas
- ✅ Definición de tipos de mensajes
- ✅ Headers incluidos correctamente
- ✅ Estructura del proyecto
- ✅ Estadísticas del código

### 2. **`test.sh`** - Pruebas Completas
**Propósito**: Pruebas funcionales completas del sistema
**Requisitos**: Colas de mensajes System V habilitadas
**Tiempo**: ~30-60 segundos

```bash
./test.sh
```

**Qué prueba:**
1. **Compilación** - Servidor y cliente
2. **Inicio del servidor** - En segundo plano
3. **Clientes múltiples** - 3 clientes simultáneos (María, Juan, Carlos)
4. **Comandos especiales** - `/list`, `/users`
5. **Mensajería** - Intercambio de mensajes entre usuarios
6. **Comando `/leave`** - Salir de salas
7. **Persistencia** - Verificar archivos de historial
8. **Logs del servidor** - Análisis de logs generados

### 3. **`cleanup.sh`** - Limpieza del Sistema
**Propósito**: Limpiar recursos, procesos y archivos temporales
**Uso**: Después de pruebas o cuando el sistema se cuelga

```bash
./cleanup.sh
```

**Qué limpia:**
- 🧹 Procesos servidor/cliente colgados
- 🧹 Colas de mensajes System V
- 🧹 Archivos compilados
- 🧹 Logs temporales
- 🧹 Opcionalmente: historial de mensajes

## 🛠️ Makefile - Targets Disponibles

### Compilación
```bash
make              # Compilar todo
make clean        # Limpiar archivos
make debug        # Compilar con debug
```

### Pruebas
```bash
make test-simple  # Pruebas básicas
make test-full    # Pruebas completas
make cleanup      # Limpieza completa
make help         # Mostrar ayuda
```

## 🎯 Escenarios de Prueba Implementados

### Escenario 1: Salas Múltiples
- **María** se une a **General**
- **Juan** se une a **General** 
- **Carlos** se une a **Deportes**
- Verificar que los mensajes solo llegan a usuarios de la misma sala

### Escenario 2: Comandos Especiales
```
> /list                    # Listar todas las salas
> join General            # Unirse a sala General  
> /users                  # Ver usuarios en sala actual
> /leave                  # Salir de sala actual
> quit                    # Salir del cliente
```

### Escenario 3: Persistencia
- Mensajes se guardan en `historial/sala_<nombre>.txt`
- Formato: `[YYYY-MM-DD HH:MM:SS] Usuario: mensaje`
- Al crear sala, se cargan últimos 10 mensajes
- Notificaciones de entrada/salida se guardan

### Escenario 4: Manejo de Errores
- Cliente sin unirse a sala intenta enviar mensaje
- Cliente intenta usar `/users` sin estar en sala
- Servidor maneja desconexiones inesperadas
- Limpieza automática de recursos

## 📊 Interpretación de Resultados

### Pruebas Exitosas
```
[✓] Compilación exitosa
[✓] Servidor iniciado correctamente  
[✓] 3 clientes conectados
[✓] Mensajes intercambiados correctamente
[✓] Archivos de persistencia creados
🎉 Sistema de chat validado exitosamente 🎉
```

### Errores Comunes

#### Error: "Function not implemented"
```
Error al crear la cola global: Function not implemented
```
**Causa**: Colas de mensajes System V no disponibles en el sistema
**Solución**: Usar `test_simple.sh` en su lugar, o habilitar System V IPC

#### Error: Procesos colgados
```
[!] 3 procesos aún ejecutándose
```
**Solución**: Ejecutar `./cleanup.sh` para terminar procesos

#### Error: Compilación fallida
```
[✗] Error compilando servidor:
undefined reference to 'pthread_create'
```
**Solución**: Verificar que se usa `-pthread` para el cliente

## 🔧 Diagnóstico y Debug

### Ver procesos del sistema
```bash
ps aux | grep -E "(servidor|cliente)"
```

### Ver colas de mensajes
```bash
ipcs -q
```

### Ver logs detallados
```bash
# Después de ejecutar test.sh
ls logs_prueba/
cat logs_prueba/servidor.log
```

### Probar manualmente paso a paso
```bash
# Terminal 1
make clean && make
./servidor

# Terminal 2  
./cliente María
> join General
> Hola mundo!

# Terminal 3
./cliente Juan  
> join General
> /users
> Hola María!
```

## 📁 Estructura de Archivos de Prueba

```
proyecto/
├── test.sh              # Pruebas completas
├── test_simple.sh       # Pruebas básicas  
├── cleanup.sh           # Limpieza del sistema
├── TESTING.md           # Esta documentación
├── logs_prueba/         # Logs generados por test.sh
│   ├── servidor.log
│   ├── output_María.log
│   ├── output_Juan.log
│   └── compilation.log
└── historial/           # Archivos de persistencia
    ├── sala_General.txt
    └── sala_Deportes.txt
```

## ✅ Lista de Verificación Manual

Antes de entregar el proyecto, verificar:

- [ ] `make clean && make` compila sin errores
- [ ] `./test_simple.sh` pasa todas las pruebas
- [ ] `./test.sh` funciona correctamente (si hay System V IPC)
- [ ] README.md está actualizado
- [ ] Código está comentado apropiadamente
- [ ] No hay archivos temporales en el repositorio
- [ ] Funcionalidades extras están documentadas

## 🚨 Solución de Problemas

### Sistema sin System V IPC
Si el sistema no soporta colas de mensajes System V:
1. Usar solo `test_simple.sh` para validar estructura
2. Documentar que el sistema está implementado correctamente
3. Considerar implementación alternativa con sockets o pipes

### Limpieza de Emergencia
Si el sistema queda en estado inconsistente:
```bash
./cleanup.sh
# Y si es necesario:
sudo ipcrm -a  # Eliminar todas las colas (cuidado!)
killall -9 servidor cliente
```