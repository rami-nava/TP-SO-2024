#include "io.h"

void* bitmap;
t_bitarray* bitmap_bitarray;
FILE* archivo_de_bloques;
int tamanio_bitmap;

static void limpiar_posiciones(uint32_t posicion_inicial, int tamanio_proceso);

//================================ ARCHIVOS DE BLOQUES ======================================================
void crear_archivo_de_bloque()
{
	uint32_t fd;
    char* path_archivo_bloques = string_from_format ("%s/bloques.dat", path_dial_fs);

    fd = open(path_archivo_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    
    free(path_archivo_bloques);

    if (fd == -1) {
        log_error(dialfs_logger,"Error al abrir el Archivo de Bloques");
    }
  
    if (ftruncate(fd, tamanio_archivo_bloques) == -1) {
        log_error(dialfs_logger,"Error al truncar el Archivo de Bloques");
    }

    close (fd);
}

FILE* levantar_archivo_bloque() {
	
    char* path_archivo_bloques = string_from_format ("%s/bloques.dat", path_dial_fs);

    archivo_de_bloques = fopen(path_archivo_bloques, "rb+");

    if (path_archivo_bloques == NULL) {
        log_error(dialfs_logger, "No se pudo abrir el archivo.");
    }
    free(path_archivo_bloques);

	return archivo_de_bloques;
}

//================================ BITMAP ======================================================
void cargar_bitmap()
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
        limpiar_posiciones(0, bytes);  // Si es la primera ejecución del sistema, se carga el bitmap con ceros, todos bloques libres
    }
    
    int sincronizacion_completada = msync(bitmap, bytes, MS_SYNC);
    if (sincronizacion_completada == -1){
        log_error(dialfs_logger, "Error al sincronizar el bitmap de memoria");
        abort();
    }

    //Guardar el tamanio del bitmap
    tamanio_bitmap = (int) bitarray_get_max_bit(bitmap_bitarray);

    //Leer contenido del bitmap
    for (int i = 0; i < 15; i++) {
    if (bitarray_test_bit(bitmap_bitarray, i)) {
            printf("Bit at index %d is 1\n", i);
    } else {
        printf("Bit at index %d is 0\n", i);
    }
    }

    close(fd_bitmap);
}

static void limpiar_posiciones(uint32_t posicion_inicial, int tamanio_proceso) {
	
    for (int i = posicion_inicial; i < posicion_inicial + tamanio_proceso; i++) {
		bitarray_clean_bit(bitmap_bitarray, i);
	}
}

void agregar_bloques(uint32_t cantidad_bloques_a_agregar, uint32_t bloque_inicial)
{
    //char* bloque_ocupado = calloc(tamanio_bloque, sizeof(char));
    char* bloque_ocupado = malloc(tamanio_bloque); 
    memset(bloque_ocupado, 'a', tamanio_bloque);

    uint32_t desplazamiento = 0;
    uint32_t bloque_libre = buscar_bloque_libre(bloque_inicial);

    marcar_bloque_ocupado(bloque_libre);

    /*
    //creo un bloque con todos sus elementos en '\0'
    for(int i = 0; i < tamanio_bloque; i++){ 
    bloque_ocupado[i] = 'a';
    }*/

    //Levanto el archivo de bloques
    archivo_de_bloques = levantar_archivo_bloque();
    
    while (desplazamiento < cantidad_bloques_a_agregar){

        // Nos poscionamos en el archivo de bloques
        fseek(archivo_de_bloques, (tamanio_bloque * bloque_libre), SEEK_SET);

        //Escribimos el bloque en el archivo
        fwrite(bloque_ocupado, sizeof(char), tamanio_bloque, archivo_de_bloques);

        //fwrite(&bloque_ocupado[0], sizeof(char), 1, archivo_de_bloques);

        bloque_libre++;
        desplazamiento++;
    }

    free(bloque_ocupado);
    fclose(archivo_de_bloques);

}

void eliminar_bloques(uint32_t cantidad_bloques_a_eliminar, uint32_t bloque_inicial) {
    
    char* bloque_vacio = calloc(tamanio_bloque, sizeof(char));
    memset(bloque_vacio, '\0', tamanio_bloque);

    // Levanto el archivo de bloques
    archivo_de_bloques = levantar_archivo_bloque();

    // Limpio el archivo de bloques con bloques vacios
    for (int i = bloque_inicial; i < cantidad_bloques_a_eliminar; i++) {
        fseek(archivo_de_bloques, (tamanio_bloque * i), SEEK_SET);
        fwrite(bloque_vacio, sizeof(char), tamanio_bloque, archivo_de_bloques);

        limpiar_posiciones(bloque_inicial, cantidad_bloques_a_eliminar);
    }

    fclose(archivo_de_bloques);
    free(bloque_vacio);
}

void compactar()
{
// TODO
}

// Busco el primer bloque libre para el bloque inicial 
uint32_t buscar_bloque_inicial_libre()
{
    for (int i = 0; i < tamanio_bitmap; i++){
        if (esta_libre(i)){
            return i;
        }
    }
    return -1;
}

uint32_t buscar_bloque_libre(uint32_t bloque_inicial)
{
    for (int i = bloque_inicial; i < tamanio_bitmap; i++){
        if (esta_libre(i)){
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
        if (esta_libre(i)) {

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

bool esta_libre(int index) {
    return bitarray_test_bit(bitmap_bitarray, index) == false;
}
