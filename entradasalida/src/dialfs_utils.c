#include "io.h"

void* bitmap;
t_bitarray* bitmap_bitarray;
FILE* archivo_de_bloques;
int tamanio_bitmap;

static void limpiar_posiciones(uint32_t posicion_inicial, int tamanio_proceso);
static bool pertenece_a_bloque_inicial(uint32_t indice_bloque);
static void limpiar_un_bloque(uint32_t indice_bloque);
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

    close(fd_bitmap);
}

static void limpiar_posiciones(uint32_t posicion_inicial, int tamanio_proceso) {
	
    for (int i = posicion_inicial; i < posicion_inicial + tamanio_proceso; i++) {
		bitarray_clean_bit(bitmap_bitarray, i);
	}
}

void leer_bitmap()
{
    for (int i = 0; i < 15; i++) {
        if (bitarray_test_bit(bitmap_bitarray, i)) {
            printf("Bit at index %d is 1\n", i);
        } else {
            printf("Bit at index %d is 0\n", i);
        }
    }
}
//================================ BLOQUES ======================================================
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
    
    while (desplazamiento < cantidad_bloques_a_agregar) {

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

void compactar(uint32_t cantidad_bloques_a_compactar, uint32_t bloque_final_archivo)
{    
    t_list *lista_indices_bloques_libres = list_create();
    int aux_recorrer_bitmap = 0;
    uint32_t indice_bloque_libre = 0;
    int cantidad_bloques_libres_encontrados = 0;
    uint32_t bloques_libres_contiguos = 0;

    // Busco los bloques libres que necesito y los guardo en la lista 
    while(cantidad_bloques_libres_encontrados <= cantidad_bloques_a_compactar) { 

        // Busco el primer bloque libre en todo el bitmap
        indice_bloque_libre = buscar_bloque_libre(bloque_final_archivo + 1); 

        //Guardo los indices de los bloques libres
        list_add(lista_indices_bloques_libres, (void*)(intptr_t)indice_bloque_libre); 

        aux_recorrer_bitmap++;
        cantidad_bloques_libres_encontrados++;
    }
    
    // Ahora reviso cuantos de los bloques libres son contiguos
    for (int i = bloque_final_archivo + 1; i < tamanio_bitmap; i++) {

        // Si el bloque no es libre, no es contiguo
        if (esta_libre(i)) {
            bloques_libres_contiguos++;
            list_remove_element(lista_indices_bloques_libres, (void*)(intptr_t)i);
        }
    }

    // Obtengo la cantidad de bloques libres no contiguos
    uint32_t bloques_a_mover = list_size(lista_indices_bloques_libres);

    // Levanto el archivo de bloques
    archivo_de_bloques = levantar_archivo_bloque();

    //junto los bloques libres
    while (bloques_a_mover > 0)
    {
        char* bloque_ocupado = malloc(tamanio_bloque); 

        // Obtengo el indice del primer bloque libre
        uint32_t bloque_libre = (uint32_t)(intptr_t)list_get(lista_indices_bloques_libres, 0);
     
        // Cantidad de bloques ocupados hasta llegar al bloque libre
        uint32_t bloques_ocupados = bloque_libre - bloque_final_archivo - bloques_libres_contiguos - 1;

        for(int i = 0; i < bloques_ocupados; i++) {
            
        // Me posiciono en el ultimo bloque ocupado por un archivo 
        fseek(archivo_de_bloques, tamanio_bloque * (bloque_libre - 1 - i), SEEK_SET);
        fread(bloque_ocupado, sizeof(char), tamanio_bloque, archivo_de_bloques);

        // Me posiciono en el bloque libre
        fseek(archivo_de_bloques, tamanio_bloque * (bloque_libre - i), SEEK_SET);
        fwrite(bloque_ocupado, sizeof(char), tamanio_bloque, archivo_de_bloques);

        // Actualizo el bloque inicial
        if(pertenece_a_bloque_inicial(bloque_libre - 1- i)) {
            //modificar_metadata_bloque_inicial(bloque_libre - i);   TODO 
        }

        } 
        // Actualizo bitmap y archivo_de_bloques
        limpiar_un_bloque(bloques_libres_contiguos + bloque_final_archivo);
        bloques_libres_contiguos++;
        marcar_bloque_ocupado(bloque_libre);

        //Elimino el primer bloque libre de la lista
        list_remove_element(lista_indices_bloques_libres, (void*)(intptr_t)bloque_libre);
        bloques_a_mover--;
    }

    list_destroy(lista_indices_bloques_libres);
    fclose(archivo_de_bloques);
}

static void limpiar_un_bloque(uint32_t indice_bloque)
{
    char* bloque_vacio = calloc(tamanio_bloque, sizeof(char));
    memset(bloque_vacio, '\0', tamanio_bloque);

    fseek(archivo_de_bloques, (tamanio_bloque * indice_bloque), SEEK_SET);
    fwrite(bloque_vacio, sizeof(char), tamanio_bloque, archivo_de_bloques);

    bitarray_clean_bit(bitmap_bitarray, indice_bloque);
}

static bool pertenece_a_bloque_inicial(uint32_t indice_bloque)
{
    uint32_t cantidad_bloques_iniciales = list_size(bloques_iniciales);

    for (int i = 0; i < cantidad_bloques_iniciales; i++) {
        if ((uint32_t)(intptr_t)list_get(bloques_iniciales, i) == indice_bloque) {
            return true;
        }
    }

    return false;
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

uint32_t buscar_bloque_libre(uint32_t bloque_inicial) //no es desde el final?
{
    for (int i = bloque_inicial; i < tamanio_bitmap; i++){
        if (esta_libre(i)){
            return i;
        }
    }
    return -1;
}

bool bloques_contiguos(uint32_t cantidad_bloques_a_buscar, uint32_t bloque_final_archivo) 
{
    uint32_t bloques_encontrados = 0;

    //Busco en el bitmap si hay suficientes bloques libres contiguos
    for (int i = bloque_final_archivo; i < tamanio_bitmap; i++) {
        if (esta_libre(i)) {

            //cantidad de bloques disponibles contiguos que encuentro
            bloques_encontrados++;

            // Hay suficientes bloques contiguos 
            if (bloques_encontrados == cantidad_bloques_a_buscar) {
                return true;
            }
        } else { // No hay suficientes bloques contiguos tenog que compactar
            return false; 
        }
    }

    return -1;
}

void marcar_bloque_ocupado(int index) {
    bitarray_set_bit(bitmap_bitarray, index);
    msync(bitmap, tamanio_bitmap, MS_SYNC);
}

bool esta_libre(int index) {
    return bitarray_test_bit(bitmap_bitarray, index) == false;
}
