# 📋 Changelog - Sistema de Chat con Colas de Mensajes

Todas las mejoras y cambios importantes del proyecto están documentados en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
y este proyecto adhiere a [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-09-29

### 🎉 Funcionalidades Iniciales Implementadas

#### ✅ **Servidor Central**
- Implementación completa del servidor con colas de mensajes System V
- Gestión automática de múltiples salas de chat
- Sistema robusto de limpieza de recursos con signal handlers
- Validación y verificación de colas de mensajes
- Logging detallado de todas las operaciones

#### ✅ **Cliente Multihilo**
- Cliente con arquitectura de hilos separados (entrada/recepción)
- Sincronización thread-safe con mutex
- Interfaz de usuario intuitiva y comandos claros
- Manejo robusto de desconexiones y errores

#### ✅ **Comandos de Usuario**
- `/list` - Listar todas las salas disponibles con número de usuarios
- `/users` - Mostrar usuarios en la sala actual
- `join <sala>` - Unirse a una sala específica (crea si no existe)
- `/leave` - Salir de la sala actual con notificaciones
- `<mensaje>` - Enviar mensaje a todos los usuarios de la sala
- `quit` - Salir del cliente con limpieza automática

#### ✅ **Sistema de Persistencia**
- Guardado automático de mensajes en archivos de texto
- Formato con timestamp: `[YYYY-MM-DD HH:MM:SS] Usuario: mensaje`
- Carga automática de historial (últimos 10 mensajes) al crear salas
- Notificaciones del sistema guardadas (entradas/salidas de usuarios)
- Organización: un archivo por sala en directorio `historial/`

#### ✅ **Funcionalidades Avanzadas**
- Notificaciones automáticas cuando usuarios entran/salen de salas
- Validación robusta de entrada y manejo de errores
- Generación de claves únicas para evitar conflictos en colas
- Verificación post-creación de colas para garantizar integridad
- Manejo de colas llenas con mensajes no bloqueantes (IPC_NOWAIT)

### 🧪 **Sistema de Pruebas**
- `test.sh` - Suite completa de pruebas automatizadas (8 fases)
- `test_simple.sh` - Pruebas básicas que funcionan sin System V IPC
- `cleanup.sh` - Script de limpieza completa del sistema
- Makefile con targets para automatización (`test-simple`, `test-full`, `cleanup`)
- Validación de compilación, funcionalidad y persistencia

### 📚 **Documentación Completa**
- `README.md` - Documentación principal con ejemplos y guías
- `TECHNICAL.md` - Documentación técnica detallada de arquitectura
- `TESTING.md` - Guía completa de pruebas y validación
- `CHANGELOG.md` - Este archivo de cambios
- Comentarios inline en código fuente

### 🛠️ **Herramientas de Desarrollo**
- Makefile con múltiples targets y ayuda
- Scripts de automatización para compilación y pruebas
- Flags de compilación apropiados (-Wall -Wextra -std=c99)
- Soporte para compilación con debug (-g -DDEBUG)

## [0.3.0] - 2025-09-29

### ✅ Agregado
- Comando `/leave` para salir de salas
- Notificaciones automáticas de entrada/salida de usuarios
- Sistema completo de persistencia de mensajes
- Carga automática de historial al crear salas

### 🔧 Mejorado
- Gestión robusta de colas de mensajes
- Validación y limpieza automática de recursos
- Manejo de errores más detallado

## [0.2.0] - 2025-09-29

### ✅ Agregado
- Comandos `/list` y `/users`
- Sistema de tipos de mensaje estructurado
- Validación de estado del cliente antes de comandos

### 🔧 Mejorado
- Protocolo de comunicación más robusto
- Interfaz de usuario más clara

## [0.1.0] - 2025-09-29

### ✅ Agregado
- Implementación básica del servidor con colas de mensajes
- Cliente básico con funcionalidad de unirse a salas
- Envío y recepción de mensajes simples
- Compilación con Makefile

### 🐛 Corregido
- Problemas de sincronización en cliente multihilo
- Errores de compilación con flags de warning
- Manejo correcto de tipos de mensaje

---

## 📋 Tipos de Cambios

- `✅ Agregado` para nuevas funcionalidades
- `🔧 Mejorado` para cambios en funcionalidades existentes  
- `🐛 Corregido` para corrección de bugs
- `🗑️ Eliminado` para funcionalidades removidas
- `🔒 Seguridad` para mejoras de seguridad

## 🎯 Próximas Versiones (Roadmap)

### [1.1.0] - Futuro
- [ ] Soporte para mensajes privados entre usuarios
- [ ] Salas con contraseña
- [ ] Límites de tiempo para salas inactivas
- [ ] Compresión de archivos de historial antiguos

### [1.2.0] - Futuro  
- [ ] Interfaz gráfica simple con ncurses
- [ ] Soporte para emojis y caracteres especiales
- [ ] Sistema de roles (admin, moderador, usuario)
- [ ] Estadísticas de uso del sistema

### [2.0.0] - Futuro
- [ ] Migración a sockets TCP para mayor portabilidad
- [ ] Soporte para archivos y multimedia
- [ ] Cifrado de mensajes
- [ ] Servidor distribuido con múltiples nodos