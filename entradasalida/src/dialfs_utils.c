#include "io.h"

void* bitmap;
t_bitarray* bitmap_bitarray;
FILE* archivo_de_bloques;
//================================ ARCHIVOS DE BLOQUES ======================================================
void crear_archivo_de_bloque(char* path_archivo_bloques, int tamanio_archivo_bloques)
{
	uint32_t fd;

    fd = open(path_archivo_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        log_error(io_logger,"Error al abrir el Archivo de Bloques");
    }
  
    if (ftruncate(fd, tamanio_archivo_bloques) == -1) {
        log_error(io_logger,"Error al truncar el Archivo de Bloques");
    }

    close (fd);
}

FILE* levantar_archivo_bloque() {
	
    char* path_archivo_bloques = NULL; // TODO
    archivo_de_bloques = fopen(path_archivo_bloques, "rb+");

    if (path_archivo_bloques == NULL) {
        log_error(io_logger, "No se pudo abrir el archivo.");
    }
	return archivo_de_bloques;
}

//================================ BITMAP ======================================================
void cargar_bitmap(int cantidad_bloques)
{
    int bytes =  cantidad_bloques / 8;  // Dividis cantidad de bloques por 8 para obtener los bytes
    bool existe_bitmap = true;   // Para chequear si el bitmap existe de una ejecución previa del sistema

    int fd_bitmap = open("bitmap.dat", O_CREAT | O_RDWR, S_IRWXU); // SI NO EXISTE EL ARCHIVO LO CREA

    if (fd_bitmap == -1){
        log_info(io_logger, "No se pudo abrir el archivo Bitmap");
    }

    ftruncate(fd_bitmap, bytes);  // SI EL ARCHIVO ES DE MENOS TAMAÑO QUE "bytes" ENTONCES LO EXTIENDE LLENANDOLO CON '\0'

    bitmap = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bitmap, 0);

    if(bitmap == MAP_FAILED){
        log_error(io_logger, "Error al usar mmap");
    }

    bitmap_bitarray = bitarray_create_with_mode((char*) bitmap, bytes , LSB_FIRST);
    
    // Si el bitmap no existe o no tiene bloques marco libres todos las posiciones del array
    if (existe_bitmap == false){
        limpiar_posiciones(bitmap_bitarray, 0, bytes);  // Si es la primera ejecución del sistema, se carga el bitmap con ceros, todos bloques libres
    }
    /*
    int contador =0;
    // Descomentar esto de abajo si se quiere checkear los valores del bitarray en pantalla
    for(int x =0;x<8000;x++){  // ESTO LO HICE PARA VER QUE HAY EN EL BITARRAY
         bitarray_test_bit(bitmap_bitarray, x);
         contador++;
    }
    log_info(io_logger, "contador: %d", contador);*/
    
    int sincronizacion_completada = msync(bitmap, bytes, MS_SYNC);
    if (sincronizacion_completada == -1){
        log_error(io_logger, "Error al sincronizar el bitmap de memoria");
        abort();
    }
    /*
    int finMmap = munmap(bitmap, bytes);
    if (finMmap == -1){
        log_info(io_logger, "Error al unmapear el bitmap de memoria");
        perror("munmap");
    }

    close(fd_bitmap);
    */
    printf("\nSE CERRO\n"); // esto lo hice para ver si llegaba a cerrar el archivo y hacer el munmap
}

uint32_t buscar_bloque_libre()
{
    int tamanio_bitmap = (int) bitarray_get_max_bit(bitmap_bitarray);

    for (int i = 0; i < tamanio_bitmap; i++){
        if (bitarray_test_bit(bitmap_bitarray, i) == false){
            bitarray_set_bit(bitmap_bitarray, i);
            msync(bitmap, tamanio_bitmap, MS_SYNC);
            return i;
        }
    }
    return -1;
}

void limpiar_posiciones(t_bitarray* un_espacio, int posicion_inicial, int tamanio_proceso) {
	int i = 0;
	for (i = posicion_inicial; i < posicion_inicial + tamanio_proceso; i++) {
		bitarray_clean_bit(un_espacio, i);
	}
}
