#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#define MAX_SALAS 10
#define MAX_USUARIOS_POR_SALA 20
#define MAX_TEXTO 256
#define MAX_NOMBRE 50
#define MAX_HISTORIAL 10
#define MAX_LINEA_ARCHIVO 512
#define DIRECTORIO_HISTORIAL "historial"

// Tipos de mensajes
#define MTYPE_JOIN 1
#define MTYPE_RESPONSE 2
#define MTYPE_MSG 3
#define MTYPE_LIST 4
#define MTYPE_USERS 5
#define MTYPE_LEAVE 6

// Estructura para los mensajes
struct mensaje {
    long mtype;         // Tipo de mensaje
    char remitente[MAX_NOMBRE];
    char texto[MAX_TEXTO];
    char sala[MAX_NOMBRE];
};

// Estructura para una sala de chat
struct sala {
    char nombre[MAX_NOMBRE];
    int cola_id;        // ID de la cola de mensajes de la sala
    int num_usuarios;
    char usuarios[MAX_USUARIOS_POR_SALA][MAX_NOMBRE];
};

struct sala salas[MAX_SALAS];
int num_salas = 0;
int cola_global;

// Declaraciones de funciones
int verificar_cola(int cola_id);
void limpiar_sala(int indice_sala);
int validar_sala_y_cola(int indice_sala);
void generar_lista_salas(char *respuesta);
void generar_lista_usuarios(int indice_sala, char *respuesta);
int remover_usuario_de_sala(int indice_sala, const char *nombre_usuario);
void notificar_usuario_salio(int indice_sala, const char *nombre_usuario);
void crear_directorio_historial();
void generar_nombre_archivo_sala(const char *nombre_sala, char *nombre_archivo);
void guardar_mensaje_en_archivo(const char *nombre_sala, const char *remitente, const char *mensaje);
void cargar_historial_sala(const char *nombre_sala);
char *obtener_timestamp();

// Función para limpiar una sala específica
void limpiar_sala(int indice_sala) {
    if (indice_sala < 0 || indice_sala >= num_salas) {
        return;
    }
    
    struct sala *s = &salas[indice_sala];
    
    // Verificar si la cola existe antes de intentar eliminarla
    if (verificar_cola(s->cola_id) == 0) {
        if (msgctl(s->cola_id, IPC_RMID, NULL) == -1) {
            printf("Error al eliminar cola de sala %s (ID: %d): %s\n", 
                   s->nombre, s->cola_id, strerror(errno));
        } else {
            printf("Cola de sala %s eliminada correctamente (ID: %d)\n", 
                   s->nombre, s->cola_id);
        }
    } else {
        printf("Advertencia: Cola de sala %s ya no existe (ID: %d)\n", 
               s->nombre, s->cola_id);
    }
}

// Función mejorada para limpiar colas al salir
void limpiar_servidor(int sig) {
    printf("\nCerrando servidor... Limpiando recursos\n");
    
    // Eliminar colas de salas primero
    for (int i = 0; i < num_salas; i++) {
        limpiar_sala(i);
    }
    
    // Eliminar cola global
    if (verificar_cola(cola_global) == 0) {
        if (msgctl(cola_global, IPC_RMID, NULL) == -1) {
            printf("Error al eliminar cola global (ID: %d): %s\n", 
                   cola_global, strerror(errno));
        } else {
            printf("Cola global eliminada correctamente (ID: %d)\n", cola_global);
        }
    } else {
        printf("Advertencia: Cola global ya no existe (ID: %d)\n", cola_global);
    }
    
    printf("Limpieza completada. Servidor cerrado.\n");
    exit(0);
}

// Función para verificar si una cola existe y es accesible
int verificar_cola(int cola_id) {
    struct msqid_ds cola_info;
    return msgctl(cola_id, IPC_STAT, &cola_info);
}

// Función para crear una nueva sala con verificación robusta
int crear_sala(const char *nombre) {
    if (num_salas >= MAX_SALAS) {
        printf("Error: Se ha alcanzado el límite máximo de salas (%d)\n", MAX_SALAS);
        return -1; // No se pueden crear más salas
    }

    // Validar nombre de sala
    if (strlen(nombre) == 0 || strlen(nombre) >= MAX_NOMBRE) {
        printf("Error: Nombre de sala inválido\n");
        return -1;
    }

    // Generar clave única para la sala
    key_t key;
    int cola_id;
    int intentos = 0;
    
    do {
        key = ftok("/tmp", num_salas + 100 + intentos);
        if (key == -1) {
            perror("Error al generar clave para la sala");
            return -1;
        }
        
        // Intentar crear la cola (debe ser nueva, no existir previamente)
        cola_id = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
        intentos++;
        
        if (intentos > 10) {
            printf("Error: No se pudo generar una clave única después de %d intentos\n", intentos);
            return -1;
        }
    } while (cola_id == -1 && errno == EEXIST);
    
    if (cola_id == -1) {
        perror("Error al crear la cola de la sala");
        return -1;
    }

    // Verificar que la cola se creó correctamente
    if (verificar_cola(cola_id) == -1) {
        printf("Error: La cola creada no es válida\n");
        msgctl(cola_id, IPC_RMID, NULL); // Limpiar cola defectuosa
        return -1;
    }

    // Inicializar la sala
    strcpy(salas[num_salas].nombre, nombre);
    salas[num_salas].cola_id = cola_id;
    salas[num_salas].num_usuarios = 0;

    printf("Nueva sala creada exitosamente: %s (cola_id: %d, key: 0x%x)\n", 
           nombre, cola_id, key);
    
    // Cargar historial de mensajes previos (si existe)
    cargar_historial_sala(nombre);
    
    num_salas++;
    return num_salas - 1; // Retornar el índice de la sala creada
}

// Función para buscar una sala por nombre
int buscar_sala(const char *nombre) {
    for (int i = 0; i < num_salas; i++) {
        if (strcmp(salas[i].nombre, nombre) == 0) {
            return i;
        }
    }
    return -1; // No encontrada
}

// Función para generar lista de salas disponibles
void generar_lista_salas(char *respuesta) {
    if (num_salas == 0) {
        strcpy(respuesta, "No hay salas disponibles.");
        return;
    }
    
    strcpy(respuesta, "SALAS DISPONIBLES:\n");
    
    for (int i = 0; i < num_salas; i++) {
        char linea_sala[MAX_NOMBRE + 50];
        snprintf(linea_sala, sizeof(linea_sala), 
                "- %s (%d usuarios)\n", 
                salas[i].nombre, 
                salas[i].num_usuarios);
        
        // Verificar que no se exceda el límite del buffer
        if (strlen(respuesta) + strlen(linea_sala) < MAX_TEXTO - 1) {
            strcat(respuesta, linea_sala);
        } else {
            strcat(respuesta, "...(lista truncada)");
            break;
        }
    }
    
    // Remover el último salto de línea si existe
    int len = strlen(respuesta);
    if (len > 0 && respuesta[len - 1] == '\n') {
        respuesta[len - 1] = '\0';
    }
}

// Función para generar lista de usuarios en una sala específica
void generar_lista_usuarios(int indice_sala, char *respuesta) {
    if (indice_sala < 0 || indice_sala >= num_salas) {
        strcpy(respuesta, "ERROR: Sala no válida.");
        return;
    }
    
    struct sala *s = &salas[indice_sala];
    
    if (s->num_usuarios == 0) {
        sprintf(respuesta, "No hay usuarios en la sala '%s'.", s->nombre);
        return;
    }
    
    sprintf(respuesta, "USUARIOS EN LA SALA '%s' (%d usuarios):\n", 
            s->nombre, s->num_usuarios);
    
    for (int i = 0; i < s->num_usuarios; i++) {
        char linea_usuario[MAX_NOMBRE + 10];
        snprintf(linea_usuario, sizeof(linea_usuario), 
                "- %s\n", s->usuarios[i]);
        
        // Verificar que no se exceda el límite del buffer
        if (strlen(respuesta) + strlen(linea_usuario) < MAX_TEXTO - 1) {
            strcat(respuesta, linea_usuario);
        } else {
            strcat(respuesta, "...(lista truncada)");
            break;
        }
    }
    
    // Remover el último salto de línea si existe
    int len = strlen(respuesta);
    if (len > 0 && respuesta[len - 1] == '\n') {
        respuesta[len - 1] = '\0';
    }
}

// Función para agregar un usuario a una sala
int agregar_usuario_a_sala(int indice_sala, const char *nombre_usuario) {
    if (indice_sala < 0 || indice_sala >= num_salas) {
        return -1;
    }

    struct sala *s = &salas[indice_sala];
    if (s->num_usuarios >= MAX_USUARIOS_POR_SALA) {
        return -1; // Sala llena
    }

    // Verificar si el usuario ya está en la sala
    for (int i = 0; i < s->num_usuarios; i++) {
        if (strcmp(s->usuarios[i], nombre_usuario) == 0) {
            return -1; // Usuario ya está en la sala
        }
    }

    // Agregar el usuario
    strcpy(s->usuarios[s->num_usuarios], nombre_usuario);
    s->num_usuarios++;
    return 0;
}

// Función para remover un usuario de una sala
int remover_usuario_de_sala(int indice_sala, const char *nombre_usuario) {
    if (indice_sala < 0 || indice_sala >= num_salas) {
        return -1;
    }

    struct sala *s = &salas[indice_sala];
    
    // Buscar el usuario en la sala
    int indice_usuario = -1;
    for (int i = 0; i < s->num_usuarios; i++) {
        if (strcmp(s->usuarios[i], nombre_usuario) == 0) {
            indice_usuario = i;
            break;
        }
    }
    
    if (indice_usuario == -1) {
        return -1; // Usuario no encontrado en la sala
    }
    
    // Mover todos los usuarios siguientes una posición hacia atrás
    for (int i = indice_usuario; i < s->num_usuarios - 1; i++) {
        strcpy(s->usuarios[i], s->usuarios[i + 1]);
    }
    
    s->num_usuarios--;
    return 0; // Usuario removido exitosamente
}

// Función para notificar a todos los usuarios que alguien salió
void notificar_usuario_salio(int indice_sala, const char *nombre_usuario) {
    if (validar_sala_y_cola(indice_sala) == -1) {
        printf("No se puede notificar salida: sala o cola inválida\n");
        return;
    }

    struct sala *s = &salas[indice_sala];
    
    // Crear mensaje de notificación
    struct mensaje msg_notif;
    msg_notif.mtype = 1; // Tipo genérico para mensajes de sala
    strcpy(msg_notif.remitente, "SISTEMA");
    strcpy(msg_notif.sala, s->nombre);
    sprintf(msg_notif.texto, "*** %s ha salido de la sala ***", nombre_usuario);
    
    // Enviar notificación solo si hay usuarios en la sala
    if (s->num_usuarios > 0) {
        if (msgsnd(s->cola_id, &msg_notif, sizeof(struct mensaje) - sizeof(long), IPC_NOWAIT) == -1) {
            if (errno == EAGAIN) {
                printf("Advertencia: Cola de sala %s llena, notificación no enviada\n", s->nombre);
            } else {
                printf("Error al enviar notificación de salida a sala %s: %s\n", 
                       s->nombre, strerror(errno));
            }
        } else {
            printf("Notificación enviada a sala %s: %s salió\n", s->nombre, nombre_usuario);
            
            // Guardar la notificación en el historial de la sala
            guardar_mensaje_en_archivo(s->nombre, "SISTEMA", msg_notif.texto);
        }
    }
}

// Función para validar sala y su cola antes de operaciones
int validar_sala_y_cola(int indice_sala) {
    if (indice_sala < 0 || indice_sala >= num_salas) {
        printf("Error: Índice de sala inválido (%d)\n", indice_sala);
        return -1;
    }

    struct sala *s = &salas[indice_sala];
    
    // Verificar que la cola de la sala existe y es accesible
    if (verificar_cola(s->cola_id) == -1) {
        printf("Error: Cola de sala %s no accesible (ID: %d): %s\n", 
               s->nombre, s->cola_id, strerror(errno));
        return -1;
    }
    
    return 0;
}

// Función mejorada para enviar un mensaje a todos los usuarios de una sala
void enviar_a_todos_en_sala(int indice_sala, struct mensaje *msg) {
    if (validar_sala_y_cola(indice_sala) == -1) {
        printf("No se puede enviar mensaje: sala o cola inválida\n");
        return;
    }

    struct sala *s = &salas[indice_sala];
    
    // Configurar el mensaje para broadcast
    msg->mtype = 1; // Tipo genérico para mensajes de sala
    
    // Intentar enviar el mensaje a la cola de la sala
    if (msgsnd(s->cola_id, msg, sizeof(struct mensaje) - sizeof(long), IPC_NOWAIT) == -1) {
        if (errno == EAGAIN) {
            printf("Advertencia: Cola de sala %s llena, mensaje no enviado\n", s->nombre);
        } else {
            printf("Error al enviar mensaje a sala %s (cola %d): %s\n", 
                   s->nombre, s->cola_id, strerror(errno));
            
            // Si la cola no existe, intentar recrearla
            if (errno == EINVAL) {
                printf("Intentando recrear cola para sala %s...\n", s->nombre);
                // Aquí podrías implementar lógica para recrear la cola si es necesario
            }
        }
    } else {
        printf("Mensaje enviado a sala %s: %s -> %s\n", 
               s->nombre, msg->remitente, msg->texto);
        
        // Guardar el mensaje en el historial de la sala
        guardar_mensaje_en_archivo(s->nombre, msg->remitente, msg->texto);
    }
}

// ==================== FUNCIONES DE PERSISTENCIA ====================

// Función para crear el directorio de historial si no existe
void crear_directorio_historial() {
    struct stat st = {0};
    
    if (stat(DIRECTORIO_HISTORIAL, &st) == -1) {
        if (mkdir(DIRECTORIO_HISTORIAL, 0755) == 0) {
            printf("Directorio de historial creado: %s\n", DIRECTORIO_HISTORIAL);
        } else {
            perror("Error al crear directorio de historial");
        }
    }
}

// Función para generar nombre de archivo basado en el nombre de la sala
void generar_nombre_archivo_sala(const char *nombre_sala, char *nombre_archivo) {
    snprintf(nombre_archivo, 256, "%s/sala_%s.txt", DIRECTORIO_HISTORIAL, nombre_sala);
}

// Función para obtener timestamp actual
char *obtener_timestamp() {
    static char timestamp[32];
    time_t tiempo_actual = time(NULL);
    struct tm *tiempo_local = localtime(&tiempo_actual);
    
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tiempo_local);
    return timestamp;
}

// Función para guardar un mensaje en el archivo de la sala
void guardar_mensaje_en_archivo(const char *nombre_sala, const char *remitente, const char *mensaje) {
    char nombre_archivo[256];
    generar_nombre_archivo_sala(nombre_sala, nombre_archivo);
    
    FILE *archivo = fopen(nombre_archivo, "a");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir el archivo %s para escritura\n", nombre_archivo);
        return;
    }
    
    char *timestamp = obtener_timestamp();
    fprintf(archivo, "[%s] %s: %s\n", timestamp, remitente, mensaje);
    fclose(archivo);
    
    printf("Mensaje guardado en %s: [%s] %s: %s\n", 
           nombre_archivo, timestamp, remitente, mensaje);
}

// Función para cargar los últimos mensajes del historial de una sala
void cargar_historial_sala(const char *nombre_sala) {
    char nombre_archivo[256];
    generar_nombre_archivo_sala(nombre_sala, nombre_archivo);
    
    FILE *archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        printf("No hay historial previo para la sala %s\n", nombre_sala);
        return;
    }
    
    // Contar líneas del archivo
    int total_lineas = 0;
    char buffer[MAX_LINEA_ARCHIVO];
    while (fgets(buffer, sizeof(buffer), archivo) != NULL) {
        total_lineas++;
    }
    
    // Si hay menos de MAX_HISTORIAL mensajes, mostrar todos
    int lineas_a_mostrar = (total_lineas < MAX_HISTORIAL) ? total_lineas : MAX_HISTORIAL;
    int lineas_a_omitir = total_lineas - lineas_a_mostrar;
    
    // Volver al inicio del archivo
    rewind(archivo);
    
    // Omitir las líneas que no queremos mostrar
    for (int i = 0; i < lineas_a_omitir; i++) {
        fgets(buffer, sizeof(buffer), archivo);
    }
    
    printf("=== HISTORIAL RECIENTE DE LA SALA %s ===\n", nombre_sala);
    
    // Leer y mostrar los últimos mensajes
    int mensajes_mostrados = 0;
    while (fgets(buffer, sizeof(buffer), archivo) != NULL && mensajes_mostrados < MAX_HISTORIAL) {
        // Remover salto de línea al final
        buffer[strcspn(buffer, "\n")] = '\0';
        printf("%s\n", buffer);
        mensajes_mostrados++;
    }
    
    if (mensajes_mostrados > 0) {
        printf("=== FIN DEL HISTORIAL (últimos %d mensajes) ===\n", mensajes_mostrados);
    }
    
    fclose(archivo);
}

int main() {
    // Configurar manejo de señales para limpieza
    signal(SIGINT, limpiar_servidor);
    signal(SIGTERM, limpiar_servidor);

    // Crear directorio para historial de mensajes
    crear_directorio_historial();

    // Crear la cola global para solicitudes de clientes
    key_t key_global = ftok("/tmp", 'A');
    cola_global = msgget(key_global, IPC_CREAT | 0666);
    if (cola_global == -1) {
        perror("Error al crear la cola global");
        exit(1);
    }

    printf("Servidor de chat iniciado. Cola global ID: %d\n", cola_global);
    printf("Esperando clientes...\n");

    struct mensaje msg;
    struct mensaje respuesta;

    while (1) {
        // Recibir mensajes de la cola global
        if (msgrcv(cola_global, &msg, sizeof(struct mensaje) - sizeof(long), 0, 0) == -1) {
            if (errno == EINTR) {
                continue; // Señal recibida, continuar
            }
            perror("Error al recibir mensaje");
            continue;
        }

        // Procesar el mensaje según su tipo
        if (msg.mtype == MTYPE_JOIN) { // JOIN
            printf("Solicitud de unirse a la sala: %s por %s\n", msg.sala, msg.remitente);

            // Buscar o crear la sala
            int indice_sala = buscar_sala(msg.sala);
            if (indice_sala == -1) {
                indice_sala = crear_sala(msg.sala);
                if (indice_sala == -1) {
                    printf("No se pudo crear la sala %s\n", msg.sala);
                    
                    // Enviar respuesta de error
                    respuesta.mtype = MTYPE_RESPONSE;
                    strcpy(respuesta.remitente, "SERVIDOR");
                    strcpy(respuesta.sala, msg.sala);
                    sprintf(respuesta.texto, "ERROR: No se pudo crear la sala %s", msg.sala);
                    
                    if (msgsnd(cola_global, &respuesta, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                        perror("Error al enviar respuesta de error");
                    }
                    continue;
                }
            }

            // Agregar el usuario a la sala
            if (agregar_usuario_a_sala(indice_sala, msg.remitente) == 0) {
                printf("Usuario %s agregado a la sala %s\n", msg.remitente, msg.sala);

                // Guardar notificación de entrada en el historial
                char mensaje_entrada[MAX_TEXTO];
                sprintf(mensaje_entrada, "*** %s se ha unido a la sala ***", msg.remitente);
                guardar_mensaje_en_archivo(msg.sala, "SISTEMA", mensaje_entrada);

                // Notificar a otros usuarios que alguien se unió (solo si hay más usuarios)
                if (salas[indice_sala].num_usuarios > 1) {
                    struct mensaje msg_notif;
                    msg_notif.mtype = 1;
                    strcpy(msg_notif.remitente, "SISTEMA");
                    strcpy(msg_notif.sala, msg.sala);
                    strcpy(msg_notif.texto, mensaje_entrada);
                    
                    if (msgsnd(salas[indice_sala].cola_id, &msg_notif, sizeof(struct mensaje) - sizeof(long), IPC_NOWAIT) == -1) {
                        if (errno != EAGAIN) {
                            printf("Error al notificar entrada de %s a sala %s: %s\n", 
                                   msg.remitente, msg.sala, strerror(errno));
                        }
                    } else {
                        printf("Notificación de entrada enviada a sala %s\n", msg.sala);
                    }
                }

                // Enviar confirmación al cliente
                respuesta.mtype = MTYPE_RESPONSE; // Tipo de respuesta
                strcpy(respuesta.remitente, "SERVIDOR");
                strcpy(respuesta.sala, msg.sala);
                sprintf(respuesta.texto, "OK:%d", salas[indice_sala].cola_id); // Enviar ID de cola
                
                if (msgsnd(cola_global, &respuesta, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                    perror("Error al enviar confirmación");
                }
            } else {
                printf("No se pudo agregar al usuario %s a la sala %s\n", msg.remitente, msg.sala);
                
                // Enviar respuesta de error
                respuesta.mtype = MTYPE_RESPONSE;
                strcpy(respuesta.remitente, "SERVIDOR");
                strcpy(respuesta.sala, msg.sala);
                sprintf(respuesta.texto, "ERROR: No se pudo unir a la sala %s", msg.sala);
                
                if (msgsnd(cola_global, &respuesta, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                    perror("Error al enviar respuesta de error");
                }
            }
        } else if (msg.mtype == MTYPE_LIST) { // LIST
            printf("Solicitud de lista de salas por %s\n", msg.remitente);

            // Generar lista de salas
            respuesta.mtype = MTYPE_RESPONSE;
            strcpy(respuesta.remitente, "SERVIDOR");
            strcpy(respuesta.sala, "");
            generar_lista_salas(respuesta.texto);
            
            if (msgsnd(cola_global, &respuesta, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                perror("Error al enviar lista de salas");
            } else {
                printf("Lista de salas enviada a %s\n", msg.remitente);
            }
        } else if (msg.mtype == MTYPE_USERS) { // USERS
            printf("Solicitud de lista de usuarios en sala %s por %s\n", msg.sala, msg.remitente);

            // Buscar la sala especificada
            int indice_sala = buscar_sala(msg.sala);
            
            respuesta.mtype = MTYPE_RESPONSE;
            strcpy(respuesta.remitente, "SERVIDOR");
            strcpy(respuesta.sala, msg.sala);
            
            if (indice_sala == -1) {
                sprintf(respuesta.texto, "ERROR: La sala '%s' no existe.", msg.sala);
            } else {
                generar_lista_usuarios(indice_sala, respuesta.texto);
            }
            
            if (msgsnd(cola_global, &respuesta, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                perror("Error al enviar lista de usuarios");
            } else {
                printf("Lista de usuarios de sala %s enviada a %s\n", msg.sala, msg.remitente);
            }
        } else if (msg.mtype == MTYPE_LEAVE) { // LEAVE
            printf("Solicitud de salir de la sala %s por %s\n", msg.sala, msg.remitente);

            // Buscar la sala especificada
            int indice_sala = buscar_sala(msg.sala);
            
            respuesta.mtype = MTYPE_RESPONSE;
            strcpy(respuesta.remitente, "SERVIDOR");
            strcpy(respuesta.sala, msg.sala);
            
            if (indice_sala == -1) {
                sprintf(respuesta.texto, "ERROR: La sala '%s' no existe.", msg.sala);
            } else {
                // Intentar remover el usuario de la sala
                if (remover_usuario_de_sala(indice_sala, msg.remitente) == 0) {
                    sprintf(respuesta.texto, "Has salido de la sala '%s' exitosamente.", msg.sala);
                    printf("Usuario %s removido de la sala %s\n", msg.remitente, msg.sala);
                    
                    // Notificar a los demás usuarios que alguien salió
                    notificar_usuario_salio(indice_sala, msg.remitente);
                } else {
                    sprintf(respuesta.texto, "ERROR: No estabas en la sala '%s'.", msg.sala);
                    printf("Usuario %s no estaba en la sala %s\n", msg.remitente, msg.sala);
                }
            }
            
            if (msgsnd(cola_global, &respuesta, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                perror("Error al enviar respuesta de leave");
            } else {
                printf("Respuesta de leave enviada a %s\n", msg.remitente);
            }
        } else if (msg.mtype == MTYPE_MSG) { // MSG
            printf("Mensaje en la sala %s de %s: %s\n", msg.sala, msg.remitente, msg.texto);

            // Buscar la sala
            int indice_sala = buscar_sala(msg.sala);
            if (indice_sala != -1) {
                // Reenviar el mensaje a todos en la sala
                enviar_a_todos_en_sala(indice_sala, &msg);
            } else {
                printf("Sala %s no encontrada\n", msg.sala);
            }
        }
    }

    return 0;
}
