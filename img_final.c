// RETO 2 - Procesamiento de Imágenes con 4 Funciones Concurrentes
// Funciones implementadas: Convolución, Rotación, Detección de Bordes, Escalado
// Cada función usa mínimo 2 hilos (pthreads) para procesamiento paralelo
// Compilar: gcc -o img_final img_final.c -pthread -lm

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct {
    int ancho;
    int alto;
    int canales;         // 1 (grises) o 3 (RGB)
    unsigned char*** pixeles; // Matriz 3D: [alto][ancho][canales]
} ImagenInfo;

void liberarImagen(ImagenInfo* info) {
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]);
            }
            free(info->pixeles[y]);
        }
        free(info->pixeles);
        info->pixeles = NULL;
    }
    info->ancho = 0;
    info->alto = 0;
    info->canales = 0;
}

int cargarImagen(const char* ruta, ImagenInfo* info) {
    int canales;
    unsigned char* datos = stbi_load(ruta, &info->ancho, &info->alto, &canales, 0);
    if (!datos) {
        fprintf(stderr, "Error al cargar imagen: %s\n", ruta);
        return 0;
    }
    info->canales = (canales == 1 || canales == 3) ? canales : 1;

    // Asignar memoria para matriz 3D
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

    stbi_image_free(datos);
    printf("Imagen cargada: %dx%d, %d canales (%s)\n", info->ancho, info->alto,
           info->canales, info->canales == 1 ? "grises" : "RGB");
    return 1;
}

void mostrarMatriz(const ImagenInfo* info) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    printf("Matriz de la imagen (primeras 10 filas):\n");
    for (int y = 0; y < info->alto && y < 10; y++) {
        for (int x = 0; x < info->ancho; x++) {
            if (info->canales == 1) {
                printf("%3u ", info->pixeles[y][x][0]);
            } else {
                printf("(%u,%u,%u) ", info->pixeles[y][x][0], 
                       info->pixeles[y][x][1], info->pixeles[y][x][2]);
            }
        }
        printf("\n");
    }
}

int guardarPNG(const ImagenInfo* info, const char* rutaSalida) {
    if (!info->pixeles) {
        fprintf(stderr, "No hay imagen para guardar.\n");
        return 0;
    }

    // Aplanar matriz 3D a 1D para stb
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

// Estructura para datos de hilos de brillo
typedef struct {
    unsigned char*** pixeles;
    int inicio;
    int fin;
    int ancho;
    int canales;
    int delta;
} BrilloArgs;

void* ajustarBrilloHilo(void* args) {
    BrilloArgs* bArgs = (BrilloArgs*)args;
    for (int y = bArgs->inicio; y < bArgs->fin; y++) {
        for (int x = 0; x < bArgs->ancho; x++) {
            for (int c = 0; c < bArgs->canales; c++) {
                int nuevo = bArgs->pixeles[y][x][c] + bArgs->delta;
                bArgs->pixeles[y][x][c] = (nuevo < 0) ? 0 : (nuevo > 255) ? 255 : nuevo;
            }
        }
    }
    return NULL;
}

void ajustarBrilloConcurrente(ImagenInfo* info, int delta) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }

    const int numHilos = 2;
    pthread_t hilos[numHilos];
    BrilloArgs args[numHilos];

    int filasPorHilo = info->alto / numHilos;
    for (int i = 0; i < numHilos; i++) {
        args[i].pixeles = info->pixeles;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i == numHilos - 1) ? info->alto : (i + 1) * filasPorHilo;
        args[i].ancho = info->ancho;
        args[i].canales = info->canales;
        args[i].delta = delta;
        pthread_create(&hilos[i], NULL, ajustarBrilloHilo, &args[i]);
    }

    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }

    printf("Brillo ajustado concurrentemente con %d hilos (delta: %+d) en imagen %s.\n", 
           numHilos, delta, info->canales == 1 ? "grises" : "RGB");
}

// ==================== FUNCIÓN 1: CONVOLUCIÓN ====================

// Estructura para datos de hilos de convolución
typedef struct {
    unsigned char*** pixelesOrigen;
    unsigned char*** pixelesDestino;
    float** kernel;
    int tamKernel;
    int ancho;
    int alto;
    int canales;
    int inicio;
    int fin;
} ConvolucionArgs;

float** generarKernelGaussiano(int tam, float sigma) {
    float** kernel = (float**)malloc(tam * sizeof(float*));
    if (!kernel) return NULL;

    for (int i = 0; i < tam; i++) {
        kernel[i] = (float*)malloc(tam * sizeof(float));
        if (!kernel[i]) {
            for (int j = 0; j < i; j++) free(kernel[j]);
            free(kernel);
            return NULL;
        }
    }

    float suma = 0.0;
    int centro = tam / 2;
    
    for (int y = 0; y < tam; y++) {
        for (int x = 0; x < tam; x++) {
            float dx = x - centro;
            float dy = y - centro;
            kernel[y][x] = exp(-(dx*dx + dy*dy) / (2.0 * sigma * sigma));
            suma += kernel[y][x];
        }
    }

    // Normalizar
    for (int y = 0; y < tam; y++) {
        for (int x = 0; x < tam; x++) {
            kernel[y][x] /= suma;
        }
    }

    return kernel;
}

void* convolucionHilo(void* args) {
    ConvolucionArgs* cArgs = (ConvolucionArgs*)args;
    int offset = cArgs->tamKernel / 2;
    
    for (int y = cArgs->inicio; y < cArgs->fin; y++) {
        for (int x = 0; x < cArgs->ancho; x++) {
            for (int c = 0; c < cArgs->canales; c++) {
                float suma = 0.0;
                
                for (int ky = 0; ky < cArgs->tamKernel; ky++) {
                    for (int kx = 0; kx < cArgs->tamKernel; kx++) {
                        int py = y + ky - offset;
                        int px = x + kx - offset;
                        
                        // Manejar bordes (clamp)
                        py = (py < 0) ? 0 : (py >= cArgs->alto) ? cArgs->alto - 1 : py;
                        px = (px < 0) ? 0 : (px >= cArgs->ancho) ? cArgs->ancho - 1 : px;
                        
                        suma += cArgs->pixelesOrigen[py][px][c] * cArgs->kernel[ky][kx];
                    }
                }
                
                int resultado = (int)(suma + 0.5);
                cArgs->pixelesDestino[y][x][c] = (resultado < 0) ? 0 : (resultado > 255) ? 255 : resultado;
            }
        }
    }
    return NULL;
}

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
    if (!kernel) return;
    
    // Crear imagen destino
    unsigned char*** pixelesDestino = (unsigned char***)malloc(info->alto * sizeof(unsigned char**));
    if (!pixelesDestino) {
        fprintf(stderr, "Error de memoria al asignar imagen destino\n");
        for (int i = 0; i < tamKernel; i++) free(kernel[i]);
        free(kernel);
        return;
    }
    
    for (int y = 0; y < info->alto; y++) {
        pixelesDestino[y] = (unsigned char**)malloc(info->ancho * sizeof(unsigned char*));
        if (!pixelesDestino[y]) {
            fprintf(stderr, "Error de memoria al asignar fila %d\n", y);
            for (int j = 0; j < y; j++) {
                for (int x = 0; x < info->ancho; x++) {
                    free(pixelesDestino[j][x]);
                }
                free(pixelesDestino[j]);
            }
            free(pixelesDestino);
            for (int i = 0; i < tamKernel; i++) free(kernel[i]);
            free(kernel);
            return;
        }
        for (int x = 0; x < info->ancho; x++) {
            pixelesDestino[y][x] = (unsigned char*)malloc(info->canales * sizeof(unsigned char));
            if (!pixelesDestino[y][x]) {
                fprintf(stderr, "Error de memoria al asignar píxel [%d][%d]\n", y, x);
                for (int j = 0; j <= y; j++) {
                    int maxX = (j == y) ? x : info->ancho;
                    for (int i = 0; i < maxX; i++) {
                        free(pixelesDestino[j][i]);
                    }
                    free(pixelesDestino[j]);
                }
                free(pixelesDestino);
                for (int i = 0; i < tamKernel; i++) free(kernel[i]);
                free(kernel);
                return;
            }
        }
    }
    
    // Configurar hilos
    const int numHilos = 2;
    pthread_t hilos[numHilos];
    ConvolucionArgs args[numHilos];
    
    int filasPorHilo = info->alto / numHilos;
    for (int i = 0; i < numHilos; i++) {
        args[i].pixelesOrigen = info->pixeles;
        args[i].pixelesDestino = pixelesDestino;
        args[i].kernel = kernel;
        args[i].tamKernel = tamKernel;
        args[i].ancho = info->ancho;
        args[i].alto = info->alto;
        args[i].canales = info->canales;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i == numHilos - 1) ? info->alto : (i + 1) * filasPorHilo;
        pthread_create(&hilos[i], NULL, convolucionHilo, &args[i]);
    }
    
    // Esperar hilos
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    
    // Reemplazar imagen original (preservando dimensiones)
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]);
            }
            free(info->pixeles[y]);
        }
        free(info->pixeles);
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

// ==================== FUNCIÓN 2: ROTACIÓN ====================

// Estructura para datos de hilos de rotación
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

void* rotacionHilo(void* args) {
    RotacionArgs* rArgs = (RotacionArgs*)args;
    
    int centroXOrigen = rArgs->anchoOrigen / 2;
    int centroYOrigen = rArgs->altoOrigen / 2;
    int centroXDestino = rArgs->anchoDestino / 2;
    int centroYDestino = rArgs->altoDestino / 2;
    
    for (int y = rArgs->inicio; y < rArgs->fin; y++) {
        for (int x = 0; x < rArgs->anchoDestino; x++) {
            // Transformación inversa
            int dx = x - centroXDestino;
            int dy = y - centroYDestino;
            
            float xOrigen = dx * rArgs->cosAngulo + dy * rArgs->sinAngulo + centroXOrigen;
            float yOrigen = -dx * rArgs->sinAngulo + dy * rArgs->cosAngulo + centroYOrigen;
            
            // Interpolación bilineal
            int x0 = (int)xOrigen;
            int y0 = (int)yOrigen;
            int x1 = x0 + 1;
            int y1 = y0 + 1;
            
            if (x0 >= 0 && x1 < rArgs->anchoOrigen && y0 >= 0 && y1 < rArgs->altoOrigen) {
                float wx = xOrigen - x0;
                float wy = yOrigen - y0;
                
                for (int c = 0; c < rArgs->canales; c++) {
                    float val = (1-wx)*(1-wy)*rArgs->pixelesOrigen[y0][x0][c] +
                               wx*(1-wy)*rArgs->pixelesOrigen[y0][x1][c] +
                               (1-wx)*wy*rArgs->pixelesOrigen[y1][x0][c] +
                               wx*wy*rArgs->pixelesOrigen[y1][x1][c];
                    rArgs->pixelesDestino[y][x][c] = (unsigned char)(val + 0.5);
                }
            } else {
                // Pixel fuera de rango, usar negro
                for (int c = 0; c < rArgs->canales; c++) {
                    rArgs->pixelesDestino[y][x][c] = 0;
                }
            }
        }
    }
    return NULL;
}

void rotarImagenConcurrente(ImagenInfo* info, float angulo) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    
    float radianes = angulo * M_PI / 180.0;
    float cosAngulo = cos(radianes);
    float sinAngulo = sin(radianes);
    
    // Calcular nuevas dimensiones
    int anchoDestino = (int)(fabs(info->ancho * cosAngulo) + fabs(info->alto * sinAngulo)) + 1;
    int altoDestino = (int)(fabs(info->ancho * sinAngulo) + fabs(info->alto * cosAngulo)) + 1;
    
    // Crear imagen destino
    unsigned char*** pixelesDestino = (unsigned char***)malloc(altoDestino * sizeof(unsigned char**));
    if (!pixelesDestino) {
        fprintf(stderr, "Error de memoria en rotación\n");
        return;
    }
    
    for (int y = 0; y < altoDestino; y++) {
        pixelesDestino[y] = (unsigned char**)malloc(anchoDestino * sizeof(unsigned char*));
        if (!pixelesDestino[y]) {
            fprintf(stderr, "Error de memoria en fila %d\n", y);
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
            pixelesDestino[y][x] = (unsigned char*)malloc(info->canales * sizeof(unsigned char));
            if (!pixelesDestino[y][x]) {
                fprintf(stderr, "Error de memoria en píxel [%d][%d]\n", y, x);
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
    
    // Configurar hilos
    const int numHilos = 2;
    pthread_t hilos[numHilos];
    RotacionArgs args[numHilos];
    
    int filasPorHilo = altoDestino / numHilos;
    for (int i = 0; i < numHilos; i++) {
        args[i].pixelesOrigen = info->pixeles;
        args[i].pixelesDestino = pixelesDestino;
        args[i].cosAngulo = cosAngulo;
        args[i].sinAngulo = sinAngulo;
        args[i].anchoOrigen = info->ancho;
        args[i].altoOrigen = info->alto;
        args[i].anchoDestino = anchoDestino;
        args[i].altoDestino = altoDestino;
        args[i].canales = info->canales;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i == numHilos - 1) ? altoDestino : (i + 1) * filasPorHilo;
        pthread_create(&hilos[i], NULL, rotacionHilo, &args[i]);
    }
    
    // Esperar hilos
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    
    // Reemplazar imagen original
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]);
            }
            free(info->pixeles[y]);
        }
        free(info->pixeles);
    }
    info->pixeles = pixelesDestino;
    info->ancho = anchoDestino;
    info->alto = altoDestino;
    
    printf("Imagen rotada concurrentemente %.1f° con %d hilos (nueva dimensión: %dx%d) en imagen %s.\n", 
           angulo, numHilos, anchoDestino, altoDestino, info->canales == 1 ? "grises" : "RGB");
}

// ==================== FUNCIÓN 3: DETECCIÓN DE BORDES ====================

// Estructura para datos de hilos de detección de bordes
typedef struct {
    unsigned char*** pixelesOrigen;
    unsigned char*** pixelesDestino;
    int inicio;
    int fin;
    int ancho;
    int alto;
} BordesArgs;

void* bordesHilo(void* args) {
    BordesArgs* bArgs = (BordesArgs*)args;
    
    // Kernels Sobel
    int sobelX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int sobelY[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    
    for (int y = bArgs->inicio; y < bArgs->fin; y++) {
        for (int x = 0; x < bArgs->ancho; x++) {
            int gx = 0, gy = 0;
            
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int py = y + ky;
                    int px = x + kx;
                    
                    // Manejar bordes (clamp)
                    py = (py < 0) ? 0 : (py >= bArgs->alto) ? bArgs->alto - 1 : py;
                    px = (px < 0) ? 0 : (px >= bArgs->ancho) ? bArgs->ancho - 1 : px;
                    
                    // Convertir a escala de grises si es necesario
                    int valor = bArgs->pixelesOrigen[py][px][0];
                    if (bArgs->pixelesOrigen[py][px] != bArgs->pixelesDestino[py][px]) { // RGB
                        valor = (bArgs->pixelesOrigen[py][px][0] + 
                                bArgs->pixelesOrigen[py][px][1] + 
                                bArgs->pixelesOrigen[py][px][2]) / 3;
                    }
                    
                    gx += valor * sobelX[ky + 1][kx + 1];
                    gy += valor * sobelY[ky + 1][kx + 1];
                }
            }
            
            int magnitud = (int)sqrt(gx*gx + gy*gy);
            bArgs->pixelesDestino[y][x][0] = (magnitud > 255) ? 255 : magnitud;
        }
    }
    return NULL;
}

void detectarBordesConcurrente(ImagenInfo* info) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    
    // Crear imagen destino (siempre grayscale)
    unsigned char*** pixelesDestino = (unsigned char***)malloc(info->alto * sizeof(unsigned char**));
    if (!pixelesDestino) {
        fprintf(stderr, "Error de memoria en detección de bordes\n");
        return;
    }
    
    for (int y = 0; y < info->alto; y++) {
        pixelesDestino[y] = (unsigned char**)malloc(info->ancho * sizeof(unsigned char*));
        if (!pixelesDestino[y]) {
            fprintf(stderr, "Error de memoria en fila %d\n", y);
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
            pixelesDestino[y][x] = (unsigned char*)malloc(1 * sizeof(unsigned char));
            if (!pixelesDestino[y][x]) {
                fprintf(stderr, "Error de memoria en píxel [%d][%d]\n", y, x);
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
    
    // Configurar hilos
    const int numHilos = 2;
    pthread_t hilos[numHilos];
    BordesArgs args[numHilos];
    
    int filasPorHilo = info->alto / numHilos;
    for (int i = 0; i < numHilos; i++) {
        args[i].pixelesOrigen = info->pixeles;
        args[i].pixelesDestino = pixelesDestino;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i == numHilos - 1) ? info->alto : (i + 1) * filasPorHilo;
        args[i].ancho = info->ancho;
        args[i].alto = info->alto;
        pthread_create(&hilos[i], NULL, bordesHilo, &args[i]);
    }
    
    // Esperar hilos
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    
    // Reemplazar imagen original (preservando dimensiones)
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]);
            }
            free(info->pixeles[y]);
        }
        free(info->pixeles);
    }
    info->pixeles = pixelesDestino;
    info->canales = 1; // Resultado siempre grayscale
    
    printf("Detección de bordes aplicada concurrentemente con %d hilos (operador Sobel) - resultado: grayscale.\n", numHilos);
}

// ==================== FUNCIÓN 4: ESCALADO ====================

// Estructura para datos de hilos de escalado
typedef struct {
    unsigned char*** pixelesOrigen;
    unsigned char*** pixelesDestino;
    int anchoOrigen;
    int altoOrigen;
    int anchoDestino;
    int altoDestino;
    int canales;
    int inicio;
    int fin;
} EscaladoArgs;

void* escaladoHilo(void* args) {
    EscaladoArgs* eArgs = (EscaladoArgs*)args;
    
    float ratioX = (float)eArgs->anchoOrigen / eArgs->anchoDestino;
    float ratioY = (float)eArgs->altoOrigen / eArgs->altoDestino;
    
    for (int y = eArgs->inicio; y < eArgs->fin; y++) {
        for (int x = 0; x < eArgs->anchoDestino; x++) {
            float xOrigen = x * ratioX;
            float yOrigen = y * ratioY;
            
            int x0 = (int)xOrigen;
            int y0 = (int)yOrigen;
            int x1 = (x0 + 1 < eArgs->anchoOrigen) ? x0 + 1 : x0;
            int y1 = (y0 + 1 < eArgs->altoOrigen) ? y0 + 1 : y0;
            
            float wx = xOrigen - x0;
            float wy = yOrigen - y0;
            
            for (int c = 0; c < eArgs->canales; c++) {
                float val = (1-wx)*(1-wy)*eArgs->pixelesOrigen[y0][x0][c] +
                           wx*(1-wy)*eArgs->pixelesOrigen[y0][x1][c] +
                           (1-wx)*wy*eArgs->pixelesOrigen[y1][x0][c] +
                           wx*wy*eArgs->pixelesOrigen[y1][x1][c];
                eArgs->pixelesDestino[y][x][c] = (unsigned char)(val + 0.5);
            }
        }
    }
    return NULL;
}

void escalarImagenConcurrente(ImagenInfo* info, int nuevoAncho, int nuevoAlto) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    
    if (nuevoAncho <= 0 || nuevoAlto <= 0) {
        printf("Las dimensiones deben ser positivas.\n");
        return;
    }
    
    // Crear imagen destino
    unsigned char*** pixelesDestino = (unsigned char***)malloc(nuevoAlto * sizeof(unsigned char**));
    if (!pixelesDestino) {
        fprintf(stderr, "Error de memoria en escalado\n");
        return;
    }
    
    for (int y = 0; y < nuevoAlto; y++) {
        pixelesDestino[y] = (unsigned char**)malloc(nuevoAncho * sizeof(unsigned char*));
        if (!pixelesDestino[y]) {
            fprintf(stderr, "Error de memoria en fila %d\n", y);
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
                fprintf(stderr, "Error de memoria en píxel [%d][%d]\n", y, x);
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
    
    // Configurar hilos
    const int numHilos = 2;
    pthread_t hilos[numHilos];
    EscaladoArgs args[numHilos];
    
    int filasPorHilo = nuevoAlto / numHilos;
    for (int i = 0; i < numHilos; i++) {
        args[i].pixelesOrigen = info->pixeles;
        args[i].pixelesDestino = pixelesDestino;
        args[i].anchoOrigen = info->ancho;
        args[i].altoOrigen = info->alto;
        args[i].anchoDestino = nuevoAncho;
        args[i].altoDestino = nuevoAlto;
        args[i].canales = info->canales;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = (i == numHilos - 1) ? nuevoAlto : (i + 1) * filasPorHilo;
        pthread_create(&hilos[i], NULL, escaladoHilo, &args[i]);
    }
    
    // Esperar hilos
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    
    // Reemplazar imagen original
    int anchoOriginal = info->ancho;
    int altoOriginal = info->alto;
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]);
            }
            free(info->pixeles[y]);
        }
        free(info->pixeles);
    }
    info->pixeles = pixelesDestino;
    info->ancho = nuevoAncho;
    info->alto = nuevoAlto;
    
    printf("Imagen escalada concurrentemente con %d hilos (de %dx%d a %dx%d) en imagen %s.\n", 
           numHilos, anchoOriginal, altoOriginal, nuevoAncho, nuevoAlto, info->canales == 1 ? "grises" : "RGB");
}

// ==================== MENÚ PRINCIPAL ====================

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

int main() {
    ImagenInfo imagen = {0, 0, 0, NULL};
    int opcion;
    char ruta[256];
    
    while (1) {
        mostrarMenu();
        if (scanf("%d", &opcion) != 1) {
            printf("Entrada inválida.\n");
            while (getchar() != '\n');
            continue;
        }
        
        switch (opcion) {
            case 1:
                printf("Ingresa la ruta del archivo PNG: ");
                scanf("%255s", ruta);
                liberarImagen(&imagen);
                cargarImagen(ruta, &imagen);
                break;
                
            case 2:
                mostrarMatriz(&imagen);
                break;
                
            case 3:
                if (!imagen.pixeles) {
                    printf("No hay imagen cargada.\n");
                    break;
                }
                printf("Nombre del archivo PNG de salida: ");
                scanf("%255s", ruta);
                guardarPNG(&imagen, ruta);
                break;
                
            case 4:
                if (!imagen.pixeles) {
                    printf("No hay imagen cargada.\n");
                    break;
                }
                int delta;
                printf("Delta de brillo (+/-): ");
                if (scanf("%d", &delta) != 1) {
                    printf("Entrada inválida.\n");
                    while (getchar() != '\n');
                    break;
                }
                ajustarBrilloConcurrente(&imagen, delta);
                break;
                
            case 5:
                if (!imagen.pixeles) {
                    printf("No hay imagen cargada.\n");
                    break;
                }
                int tamKernel;
                float sigma;
                printf("Tamaño del kernel (3 o 5): ");
                if (scanf("%d", &tamKernel) != 1) {
                    printf("Entrada inválida.\n");
                    while (getchar() != '\n');
                    break;
                }
                printf("Valor de sigma para kernel Gaussiano (ej: 1.0): ");
                if (scanf("%f", &sigma) != 1) {
                    printf("Entrada inválida.\n");
                    while (getchar() != '\n');
                    break;
                }
                aplicarConvolucionConcurrente(&imagen, tamKernel, sigma);
                break;
                
            case 6:
                if (!imagen.pixeles) {
                    printf("No hay imagen cargada.\n");
                    break;
                }
                float angulo;
                printf("Ángulo de rotación en grados (ej: 90, 180, 270): ");
                if (scanf("%f", &angulo) != 1) {
                    printf("Entrada inválida.\n");
                    while (getchar() != '\n');
                    break;
                }
                rotarImagenConcurrente(&imagen, angulo);
                break;
                
            case 7:
                if (!imagen.pixeles) {
                    printf("No hay imagen cargada.\n");
                    break;
                }
                detectarBordesConcurrente(&imagen);
                break;
                
            case 8:
                if (!imagen.pixeles) {
                    printf("No hay imagen cargada.\n");
                    break;
                }
                int nuevoAncho, nuevoAlto;
                printf("Nuevo ancho: ");
                if (scanf("%d", &nuevoAncho) != 1) {
                    printf("Entrada inválida.\n");
                    while (getchar() != '\n');
                    break;
                }
                printf("Nuevo alto: ");
                if (scanf("%d", &nuevoAlto) != 1) {
                    printf("Entrada inválida.\n");
                    while (getchar() != '\n');
                    break;
                }
                escalarImagenConcurrente(&imagen, nuevoAncho, nuevoAlto);
                break;
                
            case 9:
                printf("¡Adiós!\n");
                liberarImagen(&imagen);
                return 0;
                
            default:
                printf("Opción inválida.\n");
        }
    }
}