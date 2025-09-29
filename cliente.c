#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define MAX_TEXTO 256
#define MAX_NOMBRE 50

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

int cola_global;
int cola_sala = -1;
char nombre_usuario[MAX_NOMBRE];
char sala_actual[MAX_NOMBRE] = "";
pthread_mutex_t mutex_cola = PTHREAD_MUTEX_INITIALIZER;
volatile int ejecutando = 1;

// Función para limpiar al salir
void limpiar_cliente(int sig) {
    printf("\nSaliendo del cliente...\n");
    ejecutando = 0;
    pthread_mutex_destroy(&mutex_cola);
    exit(0);
}

// Función para el hilo que recibe mensajes
void *recibir_mensajes(void *arg) {
    struct mensaje msg;

    while (ejecutando) {
        pthread_mutex_lock(&mutex_cola);
        int cola_actual = cola_sala;
        pthread_mutex_unlock(&mutex_cola);

        if (cola_actual != -1) {
            // Recibir mensajes de la cola de la sala (no bloqueante)
            if (msgrcv(cola_actual, &msg, sizeof(struct mensaje) - sizeof(long), 1, IPC_NOWAIT) != -1) {
                // Mostrar el mensaje si no es del propio usuario
                if (strcmp(msg.remitente, nombre_usuario) != 0) {
                    printf("\n%s: %s\n> ", msg.remitente, msg.texto);
                    fflush(stdout);
                }
            } else {
                if (errno != ENOMSG) {
                    // Solo mostrar error si no es "No message of desired type"
                    if (errno != EINTR && ejecutando) {
                        perror("Error al recibir mensaje de la sala");
                    }
                }
            }
        }
        usleep(100000); // Pausa de 100ms para no consumir demasiado CPU
    }

    return NULL;
}

// Función para obtener el ID de cola de la sala a partir de la respuesta del servidor
int extraer_cola_id(const char *respuesta) {
    if (strncmp(respuesta, "OK:", 3) == 0) {
        return atoi(respuesta + 3);
    }
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: %s <nombre_usuario>\n", argv[0]);
        exit(1);
    }

    // Validar nombre de usuario
    if (strlen(argv[1]) >= MAX_NOMBRE) {
        printf("Error: El nombre de usuario es demasiado largo (máximo %d caracteres)\n", MAX_NOMBRE - 1);
        exit(1);
    }

    strcpy(nombre_usuario, argv[1]);

    // Configurar manejo de señales
    signal(SIGINT, limpiar_cliente);
    signal(SIGTERM, limpiar_cliente);

    // Conectarse a la cola global
    key_t key_global = ftok("/tmp", 'A');
    cola_global = msgget(key_global, 0666);
    if (cola_global == -1) {
        perror("Error al conectar a la cola global. ¿Está el servidor ejecutándose?");
        exit(1);
    }

    printf("Bienvenido, %s.\n", nombre_usuario);
    printf("Comandos disponibles:\n");
    printf("  /list        - Listar todas las salas disponibles\n");
    printf("  /users       - Listar usuarios en la sala actual\n");
    printf("  join <sala>  - Unirse a una sala\n");
    printf("  /leave       - Salir de la sala actual\n");
    printf("  <mensaje>    - Enviar mensaje a la sala actual\n");
    printf("  quit         - Salir del cliente\n");

    // Crear un hilo para recibir mensajes
    pthread_t hilo_receptor;
    if (pthread_create(&hilo_receptor, NULL, recibir_mensajes, NULL) != 0) {
        perror("Error al crear hilo receptor");
        exit(1);
    }

    // Separar el hilo receptor para que se limpie automáticamente
    pthread_detach(hilo_receptor);

    struct mensaje msg;
    char comando[MAX_TEXTO];

    while (ejecutando) {
        printf("> ");
        fflush(stdout);

        // Leer comando del usuario
        if (fgets(comando, MAX_TEXTO, stdin) == NULL) {
            if (feof(stdin)) {
                printf("\nEOF detectado, saliendo...\n");
                break;
            }
            continue;
        }

        comando[strcspn(comando, "\n")] = '\0'; // Eliminar el salto de línea

        // Comando para salir
        if (strcmp(comando, "quit") == 0) {
            break;
        }

        // Comando para listar salas
        if (strcmp(comando, "/list") == 0) {
            // Enviar solicitud de LIST al servidor
            msg.mtype = MTYPE_LIST;
            strcpy(msg.remitente, nombre_usuario);
            strcpy(msg.sala, "");
            strcpy(msg.texto, "");

            if (msgsnd(cola_global, &msg, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                perror("Error al enviar solicitud de LIST");
                continue;
            }

            // Esperar respuesta del servidor
            if (msgrcv(cola_global, &msg, sizeof(struct mensaje) - sizeof(long), MTYPE_RESPONSE, 0) == -1) {
                perror("Error al recibir lista de salas");
                continue;
            }

            printf("\n%s\n\n", msg.texto);
            continue;
        }

        // Comando para listar usuarios en la sala actual
        if (strcmp(comando, "/users") == 0) {
            // Verificar que el usuario esté en una sala
            if (strlen(sala_actual) == 0) {
                printf("No estás en ninguna sala. Usa 'join <sala>' para unirte a una sala primero.\n");
                continue;
            }

            // Enviar solicitud de USERS al servidor
            msg.mtype = MTYPE_USERS;
            strcpy(msg.remitente, nombre_usuario);
            strcpy(msg.sala, sala_actual);
            strcpy(msg.texto, "");

            if (msgsnd(cola_global, &msg, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                perror("Error al enviar solicitud de USERS");
                continue;
            }

            // Esperar respuesta del servidor
            if (msgrcv(cola_global, &msg, sizeof(struct mensaje) - sizeof(long), MTYPE_RESPONSE, 0) == -1) {
                perror("Error al recibir lista de usuarios");
                continue;
            }

            printf("\n%s\n\n", msg.texto);
            continue;
        }

        // Comando para salir de la sala actual
        if (strcmp(comando, "/leave") == 0) {
            // Verificar que el usuario esté en una sala
            if (strlen(sala_actual) == 0) {
                printf("No estás en ninguna sala.\n");
                continue;
            }

            // Enviar solicitud de LEAVE al servidor
            msg.mtype = MTYPE_LEAVE;
            strcpy(msg.remitente, nombre_usuario);
            strcpy(msg.sala, sala_actual);
            strcpy(msg.texto, "");

            if (msgsnd(cola_global, &msg, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                perror("Error al enviar solicitud de LEAVE");
                continue;
            }

            // Esperar respuesta del servidor
            if (msgrcv(cola_global, &msg, sizeof(struct mensaje) - sizeof(long), MTYPE_RESPONSE, 0) == -1) {
                perror("Error al recibir respuesta de leave");
                continue;
            }

            printf("%s\n", msg.texto);
            
            // Si la salida fue exitosa, limpiar estado local del cliente
            if (strncmp(msg.texto, "Has salido", 10) == 0) {
                // Desconectarse de la cola de la sala y limpiar estado
                pthread_mutex_lock(&mutex_cola);
                cola_sala = -1;
                strcpy(sala_actual, "");
                pthread_mutex_unlock(&mutex_cola);
                
                printf("Te has desconectado de la sala.\n");
            }
            continue;
        }

        if (strncmp(comando, "join ", 5) == 0) {
            // Comando para unirse a una sala
            char sala[MAX_NOMBRE];
            if (sscanf(comando, "join %49s", sala) != 1) {
                printf("Uso: join <nombre_sala>\n");
                continue;
            }

            // Enviar solicitud de JOIN al servidor
            msg.mtype = MTYPE_JOIN; // JOIN
            strcpy(msg.remitente, nombre_usuario);
            strcpy(msg.sala, sala);
            strcpy(msg.texto, "");

            if (msgsnd(cola_global, &msg, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                perror("Error al enviar solicitud de JOIN");
                continue;
            }

            // Esperar confirmación del servidor
            if (msgrcv(cola_global, &msg, sizeof(struct mensaje) - sizeof(long), MTYPE_RESPONSE, 0) == -1) {
                perror("Error al recibir confirmación");
                continue;
            }

            // Procesar respuesta del servidor
            if (strncmp(msg.texto, "ERROR:", 6) == 0) {
                printf("%s\n", msg.texto);
                continue;
            }

            // Extraer ID de cola de la respuesta
            int nueva_cola_id = extraer_cola_id(msg.texto);
            if (nueva_cola_id == -1) {
                printf("Error: Respuesta del servidor inválida\n");
                continue;
            }

            // Actualizar cola de sala de forma thread-safe
            pthread_mutex_lock(&mutex_cola);
            cola_sala = nueva_cola_id;
            strcpy(sala_actual, sala);
            pthread_mutex_unlock(&mutex_cola);

            printf("Te has unido a la sala: %s\n", sala);

        } else if (strlen(comando) > 0) {
            // Enviar un mensaje a la sala actual
            if (strlen(sala_actual) == 0) {
                printf("No estás en ninguna sala. Usa 'join <sala>' para unirte a una.\n");
                continue;
            }

            // Validar longitud del mensaje
            if (strlen(comando) >= MAX_TEXTO) {
                printf("Error: El mensaje es demasiado largo (máximo %d caracteres)\n", MAX_TEXTO - 1);
                continue;
            }

            pthread_mutex_lock(&mutex_cola);
            int cola_actual = cola_sala;
            char sala_temp[MAX_NOMBRE];
            strcpy(sala_temp, sala_actual);
            pthread_mutex_unlock(&mutex_cola);

            if (cola_actual == -1) {
                printf("Error: No hay conexión activa a una sala\n");
                continue;
            }

            msg.mtype = MTYPE_MSG; // MSG
            strcpy(msg.remitente, nombre_usuario);
            strcpy(msg.sala, sala_temp);
            strcpy(msg.texto, comando);

            if (msgsnd(cola_actual, &msg, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
                perror("Error al enviar mensaje");
                continue;
            }

            // Mostrar el propio mensaje inmediatamente
            printf("Tú: %s\n", comando);
        }
    }

    ejecutando = 0;
    printf("Saliendo del cliente...\n");
    pthread_mutex_destroy(&mutex_cola);
    return 0;
}
