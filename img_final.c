// Programa de procesamiento de imágenes en C para principiantes en Linux.
// QUÉ: Procesa imágenes PNG (escala de grises o RGB) usando matrices, con soporte
// para carga, visualización, guardado y ajuste de brillo concurrente.
// CÓMO: Usa stb_image.h para cargar PNG y stb_image_write.h para guardar PNG,
// con hilos POSIX (pthread) para el procesamiento paralelo del brillo.
// POR QUÉ: Diseñado para enseñar manejo de matrices, concurrencia y gestión de
// memoria en C, manteniendo simplicidad y robustez para principiantes.
// Dependencias: Descarga stb_image.h y stb_image_write.h desde
// https://github.com/nothings/stb
//   wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
//   wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
//
// Compilar: gcc -o img img_base.c -pthread -lm
// Ejecutar: ./img [ruta_imagen.png]

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

// QUÉ: Incluir bibliotecas stb para cargar y guardar imágenes PNG.
// CÓMO: stb_image.h lee PNG/JPG a memoria; stb_image_write.h escribe PNG.
// POR QUÉ: Son bibliotecas de un solo archivo, simples y sin dependencias externas.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// QUÉ: Estructura para almacenar la imagen (ancho, alto, canales, píxeles).
// CÓMO: Usa matriz 3D para píxeles (alto x ancho x canales), donde canales es
// 1 (grises) o 3 (RGB). Píxeles son unsigned char (0-255).
// POR QUÉ: Permite manejar tanto grises como color, con memoria dinámica para
// flexibilidad y evitar desperdicio.
typedef struct {
    int ancho;           // Ancho de la imagen en píxeles
    int alto;            // Alto de la imagen en píxeles
    int canales;         // 1 (escala de grises) o 3 (RGB)
    unsigned char*** pixeles; // Matriz 3D: [alto][ancho][canales]
} ImagenInfo;

// QUÉ: Liberar memoria asignada para la imagen.
// CÓMO: Libera cada fila y canal de la matriz 3D, luego el arreglo de filas y
// reinicia la estructura.
// POR QUÉ: Evita fugas de memoria, esencial en C para manejar recursos manualmente.
void liberarImagen(ImagenInfo* info) {
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]); // Liberar canales por píxel
            }
            free(info->pixeles[y]); // Liberar fila
        }
        free(info->pixeles); // Liberar arreglo de filas
        info->pixeles = NULL;
    }
    info->ancho = 0;
    info->alto = 0;
    info->canales = 0;
}

// QUÉ: Cargar una imagen PNG desde un archivo.
// CÓMO: Usa stbi_load para leer el archivo, detecta canales (1 o 3), y convierte
// los datos a una matriz 3D (alto x ancho x canales).
// POR QUÉ: La matriz 3D es intuitiva para principiantes y permite procesar
// píxeles y canales individualmente.
int cargarImagen(const char* ruta, ImagenInfo* info) {
    int canales;
    // QUÉ: Cargar imagen con formato original (0 canales = usar formato nativo).
    // CÓMO: stbi_load lee el archivo y llena ancho, alto y canales.
    // POR QUÉ: Respetar el formato original asegura que grises o RGB se mantengan.
    unsigned char* datos = stbi_load(ruta, &info->ancho, &info->alto, &canales, 0);
    if (!datos) {
        fprintf(stderr, "Error al cargar imagen: %s\n", ruta);
        return 0;
    }
    info->canales = (canales == 1 || canales == 3) ? canales : 1; // Forzar 1 o 3

    // QUÉ: Asignar memoria para matriz 3D.
    // CÓMO: Asignar alto filas, luego ancho columnas por fila, luego canales por píxel.
    // POR QUÉ: Estructura clara y flexible para grises (1 canal) o RGB (3 canales).
    info->pixeles = (unsigned char***)malloc(info->alto * sizeof(unsigned char**));
    if (!info->pixeles) {
        fprintf(stderr, "Error de memoria al asignar filas\n");
        stbi_image_free(datos);
        return 0;
    }
    for (int y = 0; y < info->alto; y++) {
        info->pixeles[y] = (unsigned char**)malloc(info->ancho * sizeof(unsigned char*));
        if (!info->pixeles[y]) {
            fprintf(stderr, "Error de memoria al asignar columnas\n");
            liberarImagen(info);
            stbi_image_free(datos);
            return 0;
        }
        for (int x = 0; x < info->ancho; x++) {
            info->pixeles[y][x] = (unsigned char*)malloc(info->canales * sizeof(unsigned char));
            if (!info->pixeles[y][x]) {
                fprintf(stderr, "Error de memoria al asignar canales\n");
                liberarImagen(info);
                stbi_image_free(datos);
                return 0;
            }
            // Copiar píxeles a matriz 3D
            for (int c = 0; c < info->canales; c++) {
                info->pixeles[y][x][c] = datos[(y * info->ancho + x) * info->canales + c];
            }
        }
    }

    stbi_image_free(datos); // Liberar buffer de stb
    printf("Imagen cargada: %dx%d, %d canales (%s)\n", info->ancho, info->alto,
           info->canales, info->canales == 1 ? "grises" : "RGB");
    return 1;
}

// QUÉ: Mostrar la matriz de píxeles (primeras 10 filas).
// CÓMO: Imprime los valores de los píxeles, agrupando canales por píxel (grises o RGB).
// POR QUÉ: Ayuda a visualizar la matriz para entender la estructura de datos.
void mostrarMatriz(const ImagenInfo* info) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    printf("Matriz de la imagen (primeras 10 filas):\n");
    for (int y = 0; y < info->alto && y < 10; y++) {
        for (int x = 0; x < info->ancho; x++) {
            if (info->canales == 1) {
                printf("%3u ", info->pixeles[y][x][0]); // Escala de grises
            } else {
                printf("(%3u,%3u,%3u) ", info->pixeles[y][x][0], info->pixeles[y][x][1],
                       info->pixeles[y][x][2]); // RGB
            }
        }
        printf("\n");
    }
    if (info->alto > 10) {
        printf("... (más filas)\n");
    }
}

// QUÉ: Guardar la matriz como PNG (grises o RGB).
// CÓMO: Aplana la matriz 3D a 1D y usa stbi_write_png con el número de canales correcto.
// POR QUÉ: Respeta el formato original (grises o RGB) para consistencia.
int guardarPNG(const ImagenInfo* info, const char* rutaSalida) {
    if (!info->pixeles) {
        fprintf(stderr, "No hay imagen para guardar.\n");
        return 0;
    }

    // QUÉ: Aplanar matriz 3D a 1D para stb.
    // CÓMO: Copia píxeles en orden [y][x][c] a un arreglo plano.
    // POR QUÉ: stb_write_png requiere datos contiguos.
    unsigned char* datos1D = (unsigned char*)malloc(info->ancho * info->alto * info->canales);
    if (!datos1D) {
        fprintf(stderr, "Error de memoria al aplanar imagen\n");
        return 0;
    }
    for (int y = 0; y < info->alto; y++) {
        for (int x = 0; x < info->ancho; x++) {
            for (int c = 0; c < info->canales; c++) {
                datos1D[(y * info->ancho + x) * info->canales + c] = info->pixeles[y][x][c];
            }
        }
    }

    // QUÉ: Guardar como PNG.
    // CÓMO: Usa stbi_write_png con los canales de la imagen original.
    // POR QUÉ: Mantiene el formato (grises o RGB) de la entrada.
    int resultado = stbi_write_png(rutaSalida, info->ancho, info->alto, info->canales,
                                   datos1D, info->ancho * info->canales);
    free(datos1D);
    if (resultado) {
        printf("Imagen guardada en: %s (%s)\n", rutaSalida,
               info->canales == 1 ? "grises" : "RGB");
        return 1;
    } else {
        fprintf(stderr, "Error al guardar PNG: %s\n", rutaSalida);
        return 0;
    }
}

// QUÉ: Estructura para pasar datos al hilo de ajuste de brillo.
// CÓMO: Contiene matriz, rango de filas, ancho, canales y delta de brillo.
// POR QUÉ: Los hilos necesitan datos específicos para procesar en paralelo.
typedef struct {
    unsigned char*** pixeles;
    int inicio;
    int fin;
    int ancho;
    int canales;
    int delta;
} BrilloArgs;

// QUÉ: Ajustar brillo en un rango de filas (para hilos).
// CÓMO: Suma delta a cada canal de cada píxel, con clamp entre 0-255.
// POR QUÉ: Procesa píxeles en paralelo para demostrar concurrencia.
void* ajustarBrilloHilo(void* args) {
    BrilloArgs* bArgs = (BrilloArgs*)args;
    for (int y = bArgs->inicio; y < bArgs->fin; y++) {
        for (int x = 0; x < bArgs->ancho; x++) {
            for (int c = 0; c < bArgs->canales; c++) {
                int nuevoValor = bArgs->pixeles[y][x][c] + bArgs->delta;
                bArgs->pixeles[y][x][c] = (unsigned char)(nuevoValor < 0 ? 0 :
                                                          (nuevoValor > 255 ? 255 : nuevoValor));
            }
        }
    }
    return NULL;
}

// QUÉ: Ajustar brillo de la imagen usando múltiples hilos.
// CÓMO: Divide las filas entre 2 hilos, pasa argumentos y espera con join.
// POR QUÉ: Usa concurrencia para acelerar el procesamiento y enseñar hilos.
void ajustarBrilloConcurrente(ImagenInfo* info, int delta) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }

    const int numHilos = 2; // QUÉ: Número fijo de hilos para simplicidad.
    pthread_t hilos[numHilos];
    BrilloArgs args[numHilos];
    int filasPorHilo = (int)ceil((double)info->alto / numHilos);

    // QUÉ: Configurar y lanzar hilos.
    // CÓMO: Asigna rangos de filas a cada hilo y pasa datos.
    // POR QUÉ: Divide el trabajo para procesar en paralelo.
    for (int i = 0; i < numHilos; i++) {
        args[i].pixeles = info->pixeles;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i + 1) * filasPorHilo < info->alto ? (i + 1) * filasPorHilo : info->alto;
        args[i].ancho = info->ancho;
        args[i].canales = info->canales;
        args[i].delta = delta;
        if (pthread_create(&hilos[i], NULL, ajustarBrilloHilo, &args[i]) != 0) {
            fprintf(stderr, "Error al crear hilo %d\n", i);
            return;
        }
    }

    // QUÉ: Esperar a que los hilos terminen.
    // CÓMO: Usa pthread_join para sincronizar.
    // POR QUÉ: Garantiza que todos los píxeles se procesen antes de continuar.
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    printf("Brillo ajustado concurrentemente con %d hilos (%s).\n", numHilos,
           info->canales == 1 ? "grises" : "RGB");
}

// ========================== FUNCIÓN 1: CONVOLUCIÓN (FILTRO GAUSSIANO) ==========================

// QUÉ: Estructura para pasar datos al hilo de convolución.
// CÓMO: Contiene matrices origen y destino, kernel, rango de filas y parámetros.
// POR QUÉ: Los hilos necesitan datos específicos para convolución en paralelo.
typedef struct {
    unsigned char*** pixelesOrigen;
    unsigned char*** pixelesDestino;
    float** kernel;
    int tamKernel;
    int inicio;
    int fin;
    int ancho;
    int alto;
    int canales;
} ConvolucionArgs;

// QUÉ: Generar kernel Gaussiano para convolución usando fórmula matemática.
// CÓMO: Usa fórmula G(x,y) = (1/(2πσ²)) * e^(-(x²+y²)/(2σ²)) y normaliza.
// POR QUÉ: Kernel matemáticamente correcto para filtro de desenfoque.
float** generarKernelGaussiano(int tamKernel, float sigma) {
    float** kernel = (float**)malloc(tamKernel * sizeof(float*));
    if (!kernel) {
        fprintf(stderr, "Error de memoria al asignar kernel\n");
        return NULL;
    }
    
    for (int i = 0; i < tamKernel; i++) {
        kernel[i] = (float*)malloc(tamKernel * sizeof(float));
        if (!kernel[i]) {
            fprintf(stderr, "Error de memoria al asignar fila del kernel\n");
            // Liberar memoria ya asignada
            for (int j = 0; j < i; j++) {
                free(kernel[j]);
            }
            free(kernel);
            return NULL;
        }
    }
    
    int centro = tamKernel / 2;
    float suma = 0.0f;
    float sigma2 = 2.0f * sigma * sigma;
    
    // Aplicar fórmula Gaussiana: G(x,y) = (1/(2πσ²)) * e^(-(x²+y²)/(2σ²))
    for (int y = 0; y < tamKernel; y++) {
        for (int x = 0; x < tamKernel; x++) {
            int dx = x - centro;
            int dy = y - centro;
            kernel[y][x] = exp(-(dx*dx + dy*dy) / sigma2);
            suma += kernel[y][x];
        }
    }
    
    // Normalizar kernel para que la suma sea 1
    for (int y = 0; y < tamKernel; y++) {
        for (int x = 0; x < tamKernel; x++) {
            kernel[y][x] /= suma;
        }
    }
    
    return kernel;
}

// QUÉ: Aplicar convolución en un rango de filas (para hilos).
// CÓMO: Para cada píxel, multiplica vecinos por kernel y suma con padding.
// POR QUÉ: Procesa convolución en paralelo dividiendo por filas.
void* aplicarConvolucionHilo(void* args) {
    ConvolucionArgs* cArgs = (ConvolucionArgs*)args;
    int centro = cArgs->tamKernel / 2;
    
    for (int y = cArgs->inicio; y < cArgs->fin; y++) {
        for (int x = 0; x < cArgs->ancho; x++) {
            for (int c = 0; c < cArgs->canales; c++) {
                float suma = 0.0f;
                
                // Aplicar kernel de convolución
                for (int ky = 0; ky < cArgs->tamKernel; ky++) {
                    for (int kx = 0; kx < cArgs->tamKernel; kx++) {
                        int py = y + ky - centro;
                        int px = x + kx - centro;
                        
                        // Padding por replicación de píxeles de borde
                        py = py < 0 ? 0 : (py >= cArgs->alto ? cArgs->alto - 1 : py);
                        px = px < 0 ? 0 : (px >= cArgs->ancho ? cArgs->ancho - 1 : px);
                        
                        suma += cArgs->pixelesOrigen[py][px][c] * cArgs->kernel[ky][kx];
                    }
                }
                
                // Clamp resultado entre 0-255
                cArgs->pixelesDestino[y][x][c] = (unsigned char)(suma < 0 ? 0 : 
                                                               (suma > 255 ? 255 : suma));
            }
        }
    }
    return NULL;
}

// QUÉ: Aplicar convolución (filtro Gaussiano) usando múltiples hilos.
// CÓMO: Genera kernel, crea imagen destino, divide trabajo entre hilos.
// POR QUÉ: Usa concurrencia para acelerar la convolución matricial.
void aplicarConvolucionConcurrente(ImagenInfo* info, int tamKernel, float sigma) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    
    if (tamKernel % 2 == 0 || tamKernel < 3) {
        printf("El tamaño del kernel debe ser impar y mayor o igual a 3.\n");
        return;
    }
    
    if (sigma <= 0) {
        printf("El valor de sigma debe ser positivo.\n");
        return;
    }
    
    // Generar kernel Gaussiano
    float** kernel = generarKernelGaussiano(tamKernel, sigma);
    if (!kernel) {
        return;
    }
    
    // Crear imagen destino
    unsigned char*** pixelesDestino = (unsigned char***)malloc(info->alto * sizeof(unsigned char**));
    if (!pixelesDestino) {
        fprintf(stderr, "Error de memoria al asignar imagen destino\n");
        // Liberar kernel
        for (int i = 0; i < tamKernel; i++) {
            free(kernel[i]);
        }
        free(kernel);
        return;
    }
    
    for (int y = 0; y < info->alto; y++) {
        pixelesDestino[y] = (unsigned char**)malloc(info->ancho * sizeof(unsigned char*));
        if (!pixelesDestino[y]) {
            fprintf(stderr, "Error de memoria al asignar fila %d\n", y);
            // Liberar memoria ya asignada
            for (int j = 0; j < y; j++) {
                for (int x = 0; x < info->ancho; x++) {
                    free(pixelesDestino[j][x]);
                }
                free(pixelesDestino[j]);
            }
            free(pixelesDestino);
            for (int i = 0; i < tamKernel; i++) {
                free(kernel[i]);
            }
            free(kernel);
            return;
        }
        for (int x = 0; x < info->ancho; x++) {
            pixelesDestino[y][x] = (unsigned char*)malloc(info->canales * sizeof(unsigned char));
            if (!pixelesDestino[y][x]) {
                fprintf(stderr, "Error de memoria al asignar píxel [%d][%d]\n", y, x);
                // Liberar memoria ya asignada
                for (int j = 0; j <= y; j++) {
                    int maxX = (j == y) ? x : info->ancho;
                    for (int i = 0; i < maxX; i++) {
                        free(pixelesDestino[j][i]);
                    }
                    free(pixelesDestino[j]);
                }
                free(pixelesDestino);
                for (int i = 0; i < tamKernel; i++) {
                    free(kernel[i]);
                }
                free(kernel);
                return;
            }
        }
    }
    
    const int numHilos = 2; // Mínimo 2 hilos como se especifica
    pthread_t hilos[numHilos];
    ConvolucionArgs args[numHilos];
    int filasPorHilo = (int)ceil((double)info->alto / numHilos);
    
    // Configurar y lanzar hilos
    for (int i = 0; i < numHilos; i++) {
        args[i].pixelesOrigen = info->pixeles;
        args[i].pixelesDestino = pixelesDestino;
        args[i].kernel = kernel;
        args[i].tamKernel = tamKernel;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i + 1) * filasPorHilo < info->alto ? (i + 1) * filasPorHilo : info->alto;
        args[i].ancho = info->ancho;
        args[i].alto = info->alto;
        args[i].canales = info->canales;
        
        if (pthread_create(&hilos[i], NULL, aplicarConvolucionHilo, &args[i]) != 0) {
            fprintf(stderr, "Error al crear hilo %d\n", i);
            // Liberar memoria
            for (int y = 0; y < info->alto; y++) {
                for (int x = 0; x < info->ancho; x++) {
                    free(pixelesDestino[y][x]);
                }
                free(pixelesDestino[y]);
            }
            free(pixelesDestino);
            for (int j = 0; j < tamKernel; j++) {
                free(kernel[j]);
            }
            free(kernel);
            return;
        }
    }
    
    // Esperar a que terminen todos los hilos
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    
    // Reemplazar imagen original con resultado (solo los píxeles, no las dimensiones)
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]); // Liberar canales por píxel
            }
            free(info->pixeles[y]); // Liberar fila
        }
        free(info->pixeles); // Liberar arreglo de filas
    }
    info->pixeles = pixelesDestino;
    
    // Liberar kernel
    for (int i = 0; i < tamKernel; i++) {
        free(kernel[i]);
    }
    free(kernel);
    
    printf("Convolución aplicada concurrentemente con %d hilos (kernel %dx%d, sigma=%.1f) en imagen %s.\n", 
           numHilos, tamKernel, tamKernel, sigma, info->canales == 1 ? "grises" : "RGB");
}

// ========================== FIN FUNCIÓN CONVOLUCIÓN ==========================

// ========================== FUNCIÓN 2: ROTACIÓN DE IMAGEN ==========================

// QUÉ: Estructura para pasar datos al hilo de rotación.
// CÓMO: Contiene matrices origen y destino, ángulo, dimensiones y rango.
// POR QUÉ: Los hilos necesitan datos específicos para rotación en paralelo.
typedef struct {
    unsigned char*** pixelesOrigen;
    unsigned char*** pixelesDestino;
    float cosAngulo;
    float sinAngulo;
    int anchoOrigen;
    int altoOrigen;
    int anchoDestino;
    int altoDestino;
    int canales;
    int inicio;
    int fin;
} RotacionArgs;

// QUÉ: Rotar imagen en un rango de filas (para hilos).
// CÓMO: Calcula coordenadas inversas y usa interpolación bilineal.
// POR QUÉ: Procesa rotación en paralelo dividiendo filas de imagen destino.
void* rotarImagenHilo(void* args) {
    RotacionArgs* rArgs = (RotacionArgs*)args;
    int centroX = rArgs->anchoDestino / 2;
    int centroY = rArgs->altoDestino / 2;
    int centroOrigenX = rArgs->anchoOrigen / 2;
    int centroOrigenY = rArgs->altoOrigen / 2;
    
    for (int y = rArgs->inicio; y < rArgs->fin; y++) {
        for (int x = 0; x < rArgs->anchoDestino; x++) {
            // Coordenadas relativas al centro
            int dx = x - centroX;
            int dy = y - centroY;
            
            // Rotación inversa: x' = x*cos(θ) - y*sin(θ), y' = x*sin(θ) + y*cos(θ)
            float xOrigen = dx * rArgs->cosAngulo + dy * rArgs->sinAngulo + centroOrigenX;
            float yOrigen = -dx * rArgs->sinAngulo + dy * rArgs->cosAngulo + centroOrigenY;
            
            for (int c = 0; c < rArgs->canales; c++) {
                if (xOrigen >= 0 && xOrigen < rArgs->anchoOrigen - 1 && 
                    yOrigen >= 0 && yOrigen < rArgs->altoOrigen - 1) {
                    
                    // Interpolación bilineal
                    int x1 = (int)xOrigen;
                    int y1 = (int)yOrigen;
                    int x2 = x1 + 1;
                    int y2 = y1 + 1;
                    
                    float fracX = xOrigen - x1;
                    float fracY = yOrigen - y1;
                    
                    float val1 = rArgs->pixelesOrigen[y1][x1][c] * (1 - fracX) + 
                                  rArgs->pixelesOrigen[y1][x2][c] * fracX;
                    float val2 = rArgs->pixelesOrigen[y2][x1][c] * (1 - fracX) + 
                                  rArgs->pixelesOrigen[y2][x2][c] * fracX;
                    float val = val1 * (1 - fracY) + val2 * fracY;
                    
                    rArgs->pixelesDestino[y][x][c] = (unsigned char)val;
                } else {
                    rArgs->pixelesDestino[y][x][c] = 0; // Fondo negro para píxeles fuera del rango
                }
            }
        }
    }
    return NULL;
}

// QUÉ: Rotar imagen usando múltiples hilos.
// CÓMO: Calcula nuevas dimensiones, crea imagen destino, divide trabajo entre hilos.
// POR QUÉ: Usa concurrencia para acelerar transformación geométrica matricial.
void rotarImagenConcurrente(ImagenInfo* info, float angulo) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    
    // Convertir grados a radianes
    float radianes = angulo * M_PI / 180.0f;
    float cosAngulo = cos(radianes);
    float sinAngulo = sin(radianes);
    
    // Calcular nuevas dimensiones (pueden cambiar)
    int anchoOrigen = info->ancho;
    int altoOrigen = info->alto;
    int anchoDestino = (int)(fabs(anchoOrigen * cosAngulo) + fabs(altoOrigen * sinAngulo)) + 1;
    int altoDestino = (int)(fabs(anchoOrigen * sinAngulo) + fabs(altoOrigen * cosAngulo)) + 1;
    
    // Crear NUEVA matriz 3D para imagen rotada
    unsigned char*** pixelesDestino = (unsigned char***)malloc(altoDestino * sizeof(unsigned char**));
    if (!pixelesDestino) {
        fprintf(stderr, "Error de memoria al asignar imagen rotada\n");
        return;
    }
    
    for (int y = 0; y < altoDestino; y++) {
        pixelesDestino[y] = (unsigned char**)malloc(anchoDestino * sizeof(unsigned char*));
        if (!pixelesDestino[y]) {
            fprintf(stderr, "Error de memoria al asignar fila %d\n", y);
            // Liberar memoria ya asignada
            for (int j = 0; j < y; j++) {
                for (int x = 0; x < anchoDestino; x++) {
                    free(pixelesDestino[j][x]);
                }
                free(pixelesDestino[j]);
            }
            free(pixelesDestino);
            return;
        }
        for (int x = 0; x < anchoDestino; x++) {
            pixelesDestino[y][x] = (unsigned char*)calloc(info->canales, sizeof(unsigned char));
            if (!pixelesDestino[y][x]) {
                fprintf(stderr, "Error de memoria al asignar píxel [%d][%d]\n", y, x);
                // Liberar memoria ya asignada
                for (int j = 0; j <= y; j++) {
                    int maxX = (j == y) ? x : anchoDestino;
                    for (int i = 0; i < maxX; i++) {
                        free(pixelesDestino[j][i]);
                    }
                    free(pixelesDestino[j]);
                }
                free(pixelesDestino);
                return;
            }
        }
    }
    
    const int numHilos = 2; // Mínimo 2 hilos como se especifica
    pthread_t hilos[numHilos];
    RotacionArgs args[numHilos];
    int filasPorHilo = (int)ceil((double)altoDestino / numHilos);
    
    // Configurar y lanzar hilos para procesar la NUEVA imagen
    for (int i = 0; i < numHilos; i++) {
        args[i].pixelesOrigen = info->pixeles;
        args[i].pixelesDestino = pixelesDestino;
        args[i].cosAngulo = cosAngulo;
        args[i].sinAngulo = sinAngulo;
        args[i].anchoOrigen = anchoOrigen;
        args[i].altoOrigen = altoOrigen;
        args[i].anchoDestino = anchoDestino;
        args[i].altoDestino = altoDestino;
        args[i].canales = info->canales;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i + 1) * filasPorHilo < altoDestino ? (i + 1) * filasPorHilo : altoDestino;
        
        if (pthread_create(&hilos[i], NULL, rotarImagenHilo, &args[i]) != 0) {
            fprintf(stderr, "Error al crear hilo %d\n", i);
            // Liberar memoria
            for (int y = 0; y < altoDestino; y++) {
                for (int x = 0; x < anchoDestino; x++) {
                    free(pixelesDestino[y][x]);
                }
                free(pixelesDestino[y]);
            }
            free(pixelesDestino);
            return;
        }
    }
    
    // Esperar a que terminen todos los hilos
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    
    // Liberar matriz antigua y actualizar info con la nueva
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]); // Liberar canales por píxel
            }
            free(info->pixeles[y]); // Liberar fila
        }
        free(info->pixeles); // Liberar arreglo de filas
    }
    info->pixeles = pixelesDestino;
    info->ancho = anchoDestino;
    info->alto = altoDestino;
    
    printf("Imagen rotada concurrentemente %.1f° con %d hilos (nueva dimensión: %dx%d) en imagen %s.\n", 
           angulo, numHilos, anchoDestino, altoDestino, info->canales == 1 ? "grises" : "RGB");
}

// ========================== FIN FUNCIÓN ROTACIÓN ==========================

// ========================== FUNCIÓN 3: DETECCIÓN DE BORDES (SOBEL) ==========================

// QUÉ: Estructura para pasar datos al hilo de detección de bordes.
// CÓMO: Contiene matrices origen y destino, kernels Sobel y rango de filas.
// POR QUÉ: Los hilos necesitan datos específicos para Sobel en paralelo.
typedef struct {
    unsigned char*** pixelesOrigen;
    unsigned char*** pixelesDestino;
    int inicio;
    int fin;
    int ancho;
    int alto;
    int canales;
} SobelArgs;

// QUÉ: Detectar bordes en un rango de filas usando operador Sobel (para hilos).
// CÓMO: Aplica kernels Sobel Gx y Gy, calcula magnitud del gradiente.
// POR QUÉ: Procesa detección de bordes en paralelo dividiendo por filas.
void* detectarBordesHilo(void* args) {
    SobelArgs* sArgs = (SobelArgs*)args;
    
    // Kernels Sobel para gradientes
    int sobelX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};  // Gx (horizontal)
    int sobelY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};  // Gy (vertical)
    
    for (int y = sArgs->inicio; y < sArgs->fin; y++) {
        for (int x = 0; x < sArgs->ancho; x++) {
            float gx = 0, gy = 0;
            
            // Convertir a grayscale si es RGB (promedio de canales)
            unsigned char valor;
            if (sArgs->canales == 1) {
                valor = sArgs->pixelesOrigen[y][x][0];
            } else {
                valor = (unsigned char)((sArgs->pixelesOrigen[y][x][0] + 
                                       sArgs->pixelesOrigen[y][x][1] + 
                                       sArgs->pixelesOrigen[y][x][2]) / 3);
            }
            
            // Aplicar kernels Sobel con padding (replicando píxeles de borde)
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int py = y + ky;
                    int px = x + kx;
                    
                    // Padding por replicación
                    py = py < 0 ? 0 : (py >= sArgs->alto ? sArgs->alto - 1 : py);
                    px = px < 0 ? 0 : (px >= sArgs->ancho ? sArgs->ancho - 1 : px);
                    
                    unsigned char pixelVal;
                    if (sArgs->canales == 1) {
                        pixelVal = sArgs->pixelesOrigen[py][px][0];
                    } else {
                        pixelVal = (unsigned char)((sArgs->pixelesOrigen[py][px][0] + 
                                                  sArgs->pixelesOrigen[py][px][1] + 
                                                  sArgs->pixelesOrigen[py][px][2]) / 3);
                    }
                    
                    gx += pixelVal * sobelX[ky + 1][kx + 1];
                    gy += pixelVal * sobelY[ky + 1][kx + 1];
                }
            }
            
            // Calcular magnitud: sqrt(Gx² + Gy²) y clamp a 0-255
            float magnitud = sqrt(gx * gx + gy * gy);
            unsigned char resultado = (unsigned char)(magnitud > 255 ? 255 : magnitud);
            
            // Resultado es SIEMPRE grayscale (solo canal 0)
            sArgs->pixelesDestino[y][x][0] = resultado;
        }
    }
    return NULL;
}

// QUÉ: Detectar bordes usando operador Sobel con múltiples hilos.
// CÓMO: Crea imagen destino grayscale, divide trabajo entre hilos para aplicar Sobel.
// POR QUÉ: Usa concurrencia para acelerar la detección de bordes matricial.
void detectarBordesConcurrente(ImagenInfo* info) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    
    // Crear imagen destino SIEMPRE grayscale (1 canal)
    unsigned char*** pixelesDestino = (unsigned char***)malloc(info->alto * sizeof(unsigned char**));
    if (!pixelesDestino) {
        fprintf(stderr, "Error de memoria al asignar imagen de bordes\n");
        return;
    }
    
    for (int y = 0; y < info->alto; y++) {
        pixelesDestino[y] = (unsigned char**)malloc(info->ancho * sizeof(unsigned char*));
        if (!pixelesDestino[y]) {
            fprintf(stderr, "Error de memoria al asignar fila %d\n", y);
            // Liberar memoria ya asignada
            for (int j = 0; j < y; j++) {
                for (int x = 0; x < info->ancho; x++) {
                    free(pixelesDestino[j][x]);
                }
                free(pixelesDestino[j]);
            }
            free(pixelesDestino);
            return;
        }
        for (int x = 0; x < info->ancho; x++) {
            pixelesDestino[y][x] = (unsigned char*)malloc(1 * sizeof(unsigned char)); // Solo 1 canal
            if (!pixelesDestino[y][x]) {
                fprintf(stderr, "Error de memoria al asignar píxel [%d][%d]\n", y, x);
                // Liberar memoria ya asignada
                for (int j = 0; j <= y; j++) {
                    int maxX = (j == y) ? x : info->ancho;
                    for (int i = 0; i < maxX; i++) {
                        free(pixelesDestino[j][i]);
                    }
                    free(pixelesDestino[j]);
                }
                free(pixelesDestino);
                return;
            }
        }
    }
    
    const int numHilos = 2; // Mínimo 2 hilos como se especifica
    pthread_t hilos[numHilos];
    SobelArgs args[numHilos];
    int filasPorHilo = (int)ceil((double)info->alto / numHilos);
    
    // Configurar y lanzar hilos
    for (int i = 0; i < numHilos; i++) {
        args[i].pixelesOrigen = info->pixeles;
        args[i].pixelesDestino = pixelesDestino;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i + 1) * filasPorHilo < info->alto ? (i + 1) * filasPorHilo : info->alto;
        args[i].ancho = info->ancho;
        args[i].alto = info->alto;
        args[i].canales = info->canales;
        
        if (pthread_create(&hilos[i], NULL, detectarBordesHilo, &args[i]) != 0) {
            fprintf(stderr, "Error al crear hilo %d\n", i);
            // Liberar memoria
            for (int y = 0; y < info->alto; y++) {
                for (int x = 0; x < info->ancho; x++) {
                    free(pixelesDestino[y][x]);
                }
                free(pixelesDestino[y]);
            }
            free(pixelesDestino);
            return;
        }
    }
    
    // Esperar a que terminen todos los hilos
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    
    // Liberar matriz antigua y actualizar info (SIEMPRE grayscale)
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]); // Liberar canales por píxel
            }
            free(info->pixeles[y]); // Liberar fila
        }
        free(info->pixeles); // Liberar arreglo de filas
    }
    info->pixeles = pixelesDestino;
    info->canales = 1; // Resultado es SIEMPRE grayscale
    
    printf("Detección de bordes aplicada concurrentemente con %d hilos (operador Sobel) - resultado: grayscale.\n", numHilos);
}

// ========================== FIN FUNCIÓN DETECCIÓN DE BORDES ==========================

// ========================== FUNCIÓN 4: ESCALADO DE IMAGEN (RESIZE) ==========================

// QUÉ: Estructura para pasar datos al hilo de escalado.
// CÓMO: Contiene matrices origen y destino, factores de escala y rango.
// POR QUÉ: Los hilos necesitan datos específicos para escalado en paralelo.
typedef struct {
    unsigned char*** pixelesOrigen;
    unsigned char*** pixelesDestino;
    int anchoOrigen;
    int altoOrigen;
    int anchoDestino;
    int altoDestino;
    int canales;
    float factorX;
    float factorY;
    int inicio;
    int fin;
} EscaladoArgs;

// QUÉ: Escalar imagen en un rango de filas usando interpolación bilineal (para hilos).
// CÓMO: Calcula coordenadas origen e interpola valores de píxeles.
// POR QUÉ: Procesa escalado en paralelo dividiendo filas de imagen destino.
void* escalarImagenHilo(void* args) {
    EscaladoArgs* eArgs = (EscaladoArgs*)args;
    
    for (int y = eArgs->inicio; y < eArgs->fin; y++) {
        for (int x = 0; x < eArgs->anchoDestino; x++) {
            // Calcular posición en original: scaleX = ancho_original/nuevoAncho
            float xOrigen = x * eArgs->factorX;
            float yOrigen = y * eArgs->factorY;
            
            for (int c = 0; c < eArgs->canales; c++) {
                if (xOrigen >= 0 && xOrigen < eArgs->anchoOrigen - 1 && 
                    yOrigen >= 0 && yOrigen < eArgs->altoOrigen - 1) {
                    
                    // Interpolación bilineal entre 4 píxeles vecinos
                    int x1 = (int)xOrigen;
                    int y1 = (int)yOrigen;
                    int x2 = x1 + 1;
                    int y2 = y1 + 1;
                    
                    float fracX = xOrigen - x1;
                    float fracY = yOrigen - y1;
                    
                    float val1 = eArgs->pixelesOrigen[y1][x1][c] * (1 - fracX) + 
                                  eArgs->pixelesOrigen[y1][x2][c] * fracX;
                    float val2 = eArgs->pixelesOrigen[y2][x1][c] * (1 - fracX) + 
                                  eArgs->pixelesOrigen[y2][x2][c] * fracX;
                    float val = val1 * (1 - fracY) + val2 * fracY;
                    
                    eArgs->pixelesDestino[y][x][c] = (unsigned char)val;
                } else {
                    // Píxel más cercano si está en el borde
                    int px = (int)(xOrigen + 0.5f);
                    int py = (int)(yOrigen + 0.5f);
                    px = px < 0 ? 0 : (px >= eArgs->anchoOrigen ? eArgs->anchoOrigen - 1 : px);
                    py = py < 0 ? 0 : (py >= eArgs->altoOrigen ? eArgs->altoOrigen - 1 : py);
                    eArgs->pixelesDestino[y][x][c] = eArgs->pixelesOrigen[py][px][c];
                }
            }
        }
    }
    return NULL;
}

// QUÉ: Escalar imagen usando múltiples hilos con interpolación bilineal.
// CÓMO: Calcula factores de escala, crea imagen destino, divide trabajo entre hilos.
// POR QUÉ: Usa concurrencia para acelerar el redimensionamiento matricial.
void escalarImagenConcurrente(ImagenInfo* info, int nuevoAncho, int nuevoAlto) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    
    if (nuevoAncho <= 0 || nuevoAlto <= 0) {
        printf("Las dimensiones deben ser positivas.\n");
        return;
    }
    
    // Calcular factores de escala: scaleX = ancho_original/nuevoAncho
    float factorX = (float)info->ancho / nuevoAncho;
    float factorY = (float)info->alto / nuevoAlto;
    
    // Crear NUEVA matriz 3D con nuevas dimensiones
    unsigned char*** pixelesDestino = (unsigned char***)malloc(nuevoAlto * sizeof(unsigned char**));
    if (!pixelesDestino) {
        fprintf(stderr, "Error de memoria al asignar imagen escalada\n");
        return;
    }
    
    for (int y = 0; y < nuevoAlto; y++) {
        pixelesDestino[y] = (unsigned char**)malloc(nuevoAncho * sizeof(unsigned char*));
        if (!pixelesDestino[y]) {
            fprintf(stderr, "Error de memoria al asignar fila %d\n", y);
            // Liberar memoria ya asignada
            for (int j = 0; j < y; j++) {
                for (int x = 0; x < nuevoAncho; x++) {
                    free(pixelesDestino[j][x]);
                }
                free(pixelesDestino[j]);
            }
            free(pixelesDestino);
            return;
        }
        for (int x = 0; x < nuevoAncho; x++) {
            pixelesDestino[y][x] = (unsigned char*)malloc(info->canales * sizeof(unsigned char));
            if (!pixelesDestino[y][x]) {
                fprintf(stderr, "Error de memoria al asignar píxel [%d][%d]\n", y, x);
                // Liberar memoria ya asignada
                for (int j = 0; j <= y; j++) {
                    int maxX = (j == y) ? x : nuevoAncho;
                    for (int i = 0; i < maxX; i++) {
                        free(pixelesDestino[j][i]);
                    }
                    free(pixelesDestino[j]);
                }
                free(pixelesDestino);
                return;
            }
        }
    }
    
    const int numHilos = 2; // Mínimo 2 hilos como se especifica
    pthread_t hilos[numHilos];
    EscaladoArgs args[numHilos];
    int filasPorHilo = (int)ceil((double)nuevoAlto / numHilos);
    
    // Configurar y lanzar hilos para procesar filas de la NUEVA imagen
    for (int i = 0; i < numHilos; i++) {
        args[i].pixelesOrigen = info->pixeles;
        args[i].pixelesDestino = pixelesDestino;
        args[i].anchoOrigen = info->ancho;
        args[i].altoOrigen = info->alto;
        args[i].anchoDestino = nuevoAncho;
        args[i].altoDestino = nuevoAlto;
        args[i].canales = info->canales;
        args[i].factorX = factorX;
        args[i].factorY = factorY;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i + 1) * filasPorHilo < nuevoAlto ? (i + 1) * filasPorHilo : nuevoAlto;
        
        if (pthread_create(&hilos[i], NULL, escalarImagenHilo, &args[i]) != 0) {
            fprintf(stderr, "Error al crear hilo %d\n", i);
            // Liberar memoria
            for (int y = 0; y < nuevoAlto; y++) {
                for (int x = 0; x < nuevoAncho; x++) {
                    free(pixelesDestino[y][x]);
                }
                free(pixelesDestino[y]);
            }
            free(pixelesDestino);
            return;
        }
    }
    
    // Esperar a que terminen todos los hilos
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    
    // Liberar matriz antigua y actualizar info con la nueva
    int anchoOriginal = info->ancho;
    int altoOriginal = info->alto;
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]); // Liberar canales por píxel
            }
            free(info->pixeles[y]); // Liberar fila
        }
        free(info->pixeles); // Liberar arreglo de filas
    }
    info->pixeles = pixelesDestino;
    info->ancho = nuevoAncho;
    info->alto = nuevoAlto;
    
    printf("Imagen escalada concurrentemente con %d hilos (de %dx%d a %dx%d) en imagen %s.\n", 
           numHilos, anchoOriginal, altoOriginal, nuevoAncho, nuevoAlto, info->canales == 1 ? "grises" : "RGB");
}

// ========================== FIN FUNCIÓN ESCALADO ==========================

// QUÉ: Mostrar el menú interactivo.
// CÓMO: Imprime opciones y espera entrada del usuario.
// POR QUÉ: Proporciona una interfaz simple para interactuar con el programa.
void mostrarMenu() {
    printf("\n--- Plataforma de Edición de Imágenes ---\n");
    printf("1. Cargar imagen PNG\n");
    printf("2. Mostrar matriz de píxeles\n");
    printf("3. Guardar como PNG\n");
    printf("4. Ajustar brillo (+/- valor) concurrentemente\n");
    printf("5. Aplicar convolución (filtro Gaussiano)\n");
    printf("6. Rotar imagen\n");
    printf("7. Detectar bordes (operador Sobel)\n");
    printf("8. Escalar imagen (resize)\n");
    printf("9. Salir\n");
    printf("Opción: ");
}

// QUÉ: Función principal que controla el flujo del programa.
// CÓMO: Maneja entrada CLI, ejecuta el menú en bucle y llama funciones según opción.
// POR QUÉ: Centraliza la lógica y asegura limpieza al salir.
int main(int argc, char* argv[]) {
    ImagenInfo imagen = {0, 0, 0, NULL}; // Inicializar estructura
    char ruta[256] = {0}; // Buffer para ruta de archivo

    // QUÉ: Cargar imagen desde CLI si se pasa.
    // CÓMO: Copia argv[1] y llama cargarImagen.
    // POR QUÉ: Permite ejecución directa con ./img imagen.png.
    if (argc > 1) {
        strncpy(ruta, argv[1], sizeof(ruta) - 1);
        if (!cargarImagen(ruta, &imagen)) {
            return EXIT_FAILURE;
        }
    }

    int opcion;
    while (1) {
        mostrarMenu();
        // QUÉ: Leer opción del usuario.
        // CÓMO: Usa scanf y limpia el buffer para evitar bucles infinitos.
        // POR QUÉ: Manejo robusto de entrada evita errores comunes.
        if (scanf("%d", &opcion) != 1) {
            while (getchar() != '\n');
            printf("Entrada inválida.\n");
            continue;
        }
        while (getchar() != '\n'); // Limpiar buffer

        switch (opcion) {
            case 1: { // Cargar imagen
                printf("Ingresa la ruta del archivo PNG: ");
                if (fgets(ruta, sizeof(ruta), stdin) == NULL) {
                    printf("Error al leer ruta.\n");
                    continue;
                }
                ruta[strcspn(ruta, "\n")] = 0; // Eliminar salto de línea
                liberarImagen(&imagen); // Liberar imagen previa
                if (!cargarImagen(ruta, &imagen)) {
                    continue;
                }
                break;
            }
            case 2: // Mostrar matriz
                mostrarMatriz(&imagen);
                break;
            case 3: { // Guardar PNG
                char salida[256];
                printf("Nombre del archivo PNG de salida: ");
                if (fgets(salida, sizeof(salida), stdin) == NULL) {
                    printf("Error al leer ruta.\n");
                    continue;
                }
                salida[strcspn(salida, "\n")] = 0;
                guardarPNG(&imagen, salida);
                break;
            }
            case 4: { // Ajustar brillo
                int delta;
                printf("Valor de ajuste de brillo (+ para más claro, - para más oscuro): ");
                if (scanf("%d", &delta) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                while (getchar() != '\n');
                ajustarBrilloConcurrente(&imagen, delta);
                break;
            }
            case 5: { // Aplicar convolución
                int tamKernel;
                float sigma;
                printf("Tamaño del kernel (3 o 5): ");
                if (scanf("%d", &tamKernel) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                printf("Valor de sigma para kernel Gaussiano (ej: 1.0): ");
                if (scanf("%f", &sigma) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                while (getchar() != '\n');
                aplicarConvolucionConcurrente(&imagen, tamKernel, sigma);
                break;
            }
            case 6: { // Rotar imagen
                float angulo;
                printf("Ángulo de rotación en grados (ej: 90, 180, 270): ");
                if (scanf("%f", &angulo) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                while (getchar() != '\n');
                rotarImagenConcurrente(&imagen, angulo);
                break;
            }
            case 7: { // Detectar bordes
                detectarBordesConcurrente(&imagen);
                break;
            }
            case 8: { // Escalar imagen
                int nuevoAncho, nuevoAlto;
                printf("Nuevo ancho: ");
                if (scanf("%d", &nuevoAncho) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                printf("Nuevo alto: ");
                if (scanf("%d", &nuevoAlto) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                while (getchar() != '\n');
                escalarImagenConcurrente(&imagen, nuevoAncho, nuevoAlto);
                break;
            }
            case 9: // Salir
                liberarImagen(&imagen);
                printf("¡Adiós!\n");
                return EXIT_SUCCESS;
            default:
                printf("Opción inválida.\n");
        }
    }
    liberarImagen(&imagen);
    return EXIT_SUCCESS;
}