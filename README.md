# RETO 2: Extensión Concurrente de Procesamiento de Imágenes

## Descripción
Extensión del programa base de procesamiento de imágenes PNG con 4 nuevas funciones de procesamiento concurrente usando pthreads.

## Compilación
```bash
gcc -o img img_base.c -pthread -lm
```

## Ejecución
```bash
./img [imagen.png]  # Opcional: cargar imagen al iniciar
```

## Funciones Implementadas

### 1. Convolución (Filtro de Desenfoque Gaussiano)
- **QUÉ**: Aplica un kernel de convolución Gaussiano para suavizar la imagen
- **CÓMO**: Genera kernel matemáticamente correcto, aplica convolución con padding de borde
- **CONCURRENCIA**: 4 hilos dividen el procesamiento por filas
- **PARÁMETROS**: 
  - Tamaño del kernel (debe ser impar: 3, 5, 7, etc.)
  - Valor sigma para distribución Gaussiana (ej: 1.0)

### 2. Rotación de Imagen
- **QUÉ**: Rota la imagen en un ángulo especificado (grados)
- **CÓMO**: Usa transformaciones matriciales con interpolación bilineal
- **CONCURRENCIA**: 4 hilos procesan filas de la imagen destino en paralelo
- **PARÁMETROS**: 
  - Ángulo en grados (ej: 90, 180, 270, o valores arbitrarios)
- **NOTA**: Las dimensiones de la imagen cambian para contener toda la imagen rotada

### 3. Detección de Bordes (Operador Sobel)
- **QUÉ**: Detecta bordes usando kernels Sobel para gradientes horizontal y vertical
- **CÓMO**: Aplica convolución con kernels Sobel, calcula magnitud del gradiente
- **CONCURRENCIA**: 4 hilos dividen el procesamiento por filas
- **PARÁMETROS**: Ninguno (automático)
- **NOTA**: Convierte imágenes RGB a escala de grises automáticamente

### 4. Escalado de Imagen (Resize)
- **QUÉ**: Redimensiona la imagen a nuevas dimensiones
- **CÓMO**: Usa interpolación bilineal para calcular valores de píxeles
- **CONCURRENCIA**: 4 hilos procesan filas de la imagen destino en paralelo
- **PARÁMETROS**: 
  - Nuevo ancho (píxeles)
  - Nuevo alto (píxeles)

## Características Técnicas

### Concurrencia
- Cada función usa **mínimo 4 hilos** (excepto brillo original que usa 2)
- División del trabajo por **filas** para evitar race conditions
- Sincronización con `pthread_join()` para garantizar finalización
- Manejo de errores en `pthread_create()`

### Compatibilidad
- **Escala de grises** (1 canal) y **RGB** (3 canales)
- Mantiene formato original de la imagen
- Gestión de memoria dinámica sin fugas
- Manejo de bordes con padding por replicación

### Operaciones Matriciales
- **Convolución**: Operaciones locales con kernels
- **Rotación**: Transformaciones geométricas con matrices
- **Sobel**: Convolución con kernels de gradiente
- **Escalado**: Interpolación bilineal para redimensionamiento

## Estructura del Código

### Funciones Base (Sin modificar)
- `cargarImagen()` - Carga PNG usando stb_image
- `liberarImagen()` - Gestión de memoria
- `mostrarMatriz()` - Visualización de píxeles
- `guardarPNG()` - Exporta usando stb_image_write
- `ajustarBrilloConcurrente()` - Función original de brillo

### Nuevas Funciones Principales
- `aplicarConvolucionConcurrente()`
- `rotarImagenConcurrente()` 
- `detectarBordesConcurrente()`
- `escalarImagenConcurrente()`

### Estructuras de Datos para Hilos
- `ConvolucionArgs` - Datos para convolución
- `RotacionArgs` - Datos para rotación  
- `SobelArgs` - Datos para detección de bordes
- `EscaladoArgs` - Datos para escalado

### Funciones de Hilos
- `aplicarConvolucionHilo()`
- `rotarImagenHilo()`
- `detectarBordesHilo()`
- `escalarImagenHilo()`

## Menú Interactivo
```
1. Cargar imagen PNG
2. Mostrar matriz de píxeles
3. Guardar como PNG
4. Ajustar brillo (+/- valor) concurrentemente
5. Aplicar convolución (filtro Gaussiano)      [NUEVO]
6. Rotar imagen                                [NUEVO]
7. Detectar bordes (operador Sobel)           [NUEVO]
8. Escalar imagen (resize)                    [NUEVO]
9. Salir
```

## Dependencias
- `stb_image.h` - Carga de imágenes
- `stb_image_write.h` - Escritura de imágenes
- `pthread` - Concurrencia POSIX
- `math.h` - Funciones matemáticas

## Ejemplo de Uso
```bash
./img
# Menú interactivo:
# 1. Cargar imagen
# 5. Aplicar convolución -> Kernel: 5, Sigma: 1.5
# 6. Rotar imagen -> Ángulo: 45
# 7. Detectar bordes
# 8. Escalar imagen -> 800x600
# 3. Guardar resultado
```

## Limitaciones y Consideraciones
- Las imágenes se procesan en memoria (RAM)
- Rotación puede cambiar dimensiones significativamente
- Detección de bordes convierte RGB a escala de grises
- Escalado con interpolación bilineal (calidad media)
- Convolución requiere kernels de tamaño impar