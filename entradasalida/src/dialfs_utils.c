#include "io.h"

void* bitmap;
void* archivo_de_bloques_mapeado;
t_bitarray* bitmap_bitarray;
FILE* archivo_de_bloques;
int tamanio_bitmap;

//================================ ARCHIVOS DE BLOQUES ======================================================
void crear_archivo_de_bloque(char* path_dial_fs, int tamanio_archivo_bloques)
{
	uint32_t fd;
    char* path_archivo_bloques = string_from_format ("%s/bloques.dat", path_dial_fs);

    printf("CREANDO ARCHIVO DE BLOQUES %s\n", path_archivo_bloques);

    fd = open(path_archivo_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    
    free(path_archivo_bloques);

    if (fd == -1) {
        log_error(dialfs_logger,"Error al abrir el Archivo de Bloques");
    }
  
    if (ftruncate(fd, tamanio_archivo_bloques) == -1) {
        log_error(dialfs_logger,"Error al truncar el Archivo de Bloques");
    }

    archivo_de_bloques_mapeado = mmap(&archivo_de_bloques_mapeado, tamanio_archivo_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    close (fd);
}

FILE* levantar_archivo_bloque(char* path_dial_fs) {
	
    char* path_archivo_bloques = string_from_format ("%s/bloques.dat", path_dial_fs);

    archivo_de_bloques = fopen(path_archivo_bloques, "rb+");

    if (path_archivo_bloques == NULL) {
        log_error(dialfs_logger, "No se pudo abrir el archivo.");
    }
    free(path_archivo_bloques);

	return archivo_de_bloques;
}

//================================ BITMAP ======================================================
void cargar_bitmap(int cantidad_bloques, char* path_dial_fs)
{
    char* path_bitmap = string_from_format ("%s/bitmap.dat", path_dial_fs);
    int bytes =  cantidad_bloques / 8;  // Dividis cantidad de bloques por 8 para obtener los bytes
    bool existe_bitmap = true;   // Para chequear si el bitmap existe de una ejecución previa del sistema

    int fd_bitmap = open(path_bitmap, O_CREAT | O_RDWR, S_IRWXU); // SI NO EXISTE EL ARCHIVO LO CREA
    free(path_bitmap);

    if (fd_bitmap == -1){
        log_info(dialfs_logger, "No se pudo abrir el archivo Bitmap");
    }

    ftruncate(fd_bitmap, bytes);  // SI EL ARCHIVO ES DE MENOS TAMAÑO QUE "bytes" ENTONCES LO EXTIENDE LLENANDOLO CON '\0'

    bitmap = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bitmap, 0);

    if(bitmap == MAP_FAILED){
        log_error(dialfs_logger, "Error al usar mmap");
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
    log_info(dialfs_logger, "contador: %d", contador);*/
    
    int sincronizacion_completada = msync(bitmap, bytes, MS_SYNC);
    if (sincronizacion_completada == -1){
        log_error(dialfs_logger, "Error al sincronizar el bitmap de memoria");
        abort();
    }
    /*
    int finMmap = munmap(bitmap, bytes);
    if (finMmap == -1){
        log_info(dialfs_logger, "Error al unmapear el bitmap de memoria");
        perror("munmap");
    }

    */

    //guardo el tamanio del bitmap
    tamanio_bitmap = (int) bitarray_get_max_bit(bitmap_bitarray);

    close(fd_bitmap);
}

void limpiar_posiciones(t_bitarray* un_espacio, int posicion_inicial, int tamanio_proceso) {
	int i = 0;
	for (i = posicion_inicial; i < posicion_inicial + tamanio_proceso; i++) {
		bitarray_clean_bit(un_espacio, i);
	}
}

void agregar_bloque(uint32_t bloque_a_agregar)
{
// TODO
}

void eliminar_bloque(uint32_t bloque_a_eliminar)
{
// TODO
}

void ocupar_un_bloque_del_fs()
{
// TODO
}

void liberar_un_bloque_del_fs()
{
// TODO
}

void compactar()
{
// TODO
}

//busco en todo el bitmap y devuelvo el primer bloque libre
uint32_t buscar_bloque_libre()
{
    for (int i = 0; i < tamanio_bitmap; i++){
        if (bitarray_test_bit(bitmap_bitarray, i) == false){

            //lo marco como ocupado
            bitarray_set_bit(bitmap_bitarray, i);

            //actualizo el bitmap para que se guarde como ocupado
            msync(bitmap, tamanio_bitmap, MS_SYNC);
            return i;
        }
    }
    return -1;
}

//busco si hay esta cantidad de bloques contiguos
bool bloques_contiguos(uint32_t cantidad_bloques_a_buscar) {
    uint32_t bloques_encontrados = 0;

    //i es la posicion donde estoy dentro del bitmap
    for (int i = 0; i < tamanio_bitmap; i++) {
        if (bitarray_test_bit(bitmap_bitarray, i) == false) {

            //cantidad de bloques disponibles contiguos que encuentro
            bloques_encontrados++;

            if (bloques_encontrados == cantidad_bloques_a_buscar) {

                //j =  |x|x| | | | | EJ: si buscaba 4 bloques, cuando los consiga i = 6
                //pero yo empiezo asignar desde el bloque 3. Hago 6 - 4 y como empiezo desde el primer bloque libre, +1
                for (int j = i - cantidad_bloques_a_buscar + 1; j <= i; j++) {

                    //marco todos los consecutivos como ocupados y actualizo el bitmap 
                    marcar_bloque_ocupado(j);
                }
                
                //devuelvo que habia contiguos y los pude asignar
                return true;
            }
        } else {
            //reinicio la busqueda si encuentro un bloque ocupado pero sigo desde la posicion donde estoy
            bloques_encontrados = 0; 
        }
    }

    //si no hay suficientes bloques contiguos
    return false; 
}

void marcar_bloque_ocupado(int index) {
    bitarray_set_bit(bitmap_bitarray, index);
    msync(bitmap, tamanio_bitmap, MS_SYNC);
}

uint32_t buscar_bloque_en_fs(uint32_t cantidad_bloques, uint32_t bloque_inicial)
{
    //TODO
    for(int i = bloque_inicial; i < cantidad_bloques; i++){
        if (bitarray_test_bit(bitmap_bitarray, i) == true){
            return i;
        }
    }
    return 0;
}

