#include "io.h"

static char *ip_kernel;
static char *puerto_kernel;
static char *ip_memoria;
static char *puerto_memoria;
static int socket_kernel;
static int socket_memoria;
char* path_dial_fs;
int tamanio_bloque;
int cantidad_bloques;
int tamanio_archivo_bloques;
int tiempo_unidad_trabajo;
int retraso_compactacion;
t_log *dialfs_logger;

static void recibir_peticiones_de_kernel();
static void consumir_una_unidad_de_tiempo_de_trabajo();
static void aviso_de_operacion_finalizada_a_kernel(int proceso_conectado);
static void crear_archivo(char *nombre_archivo);
static void eliminar_archivo(char *nombre_archivo);
static void truncar_archivo(char *nombre_archivo, uint32_t tamanio_nuevo);
static void ampliar_archivo(uint32_t tamanio_nuevo, uint32_t tamanio_actual, uint32_t bloque_inicial);
static void reducir_archivo(uint32_t tamanio_nuevo, uint32_t tamanio_actual, uint32_t bloque_inicial);
static void leer_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t bytes_a_leer, uint32_t direccion_fisica);
static void escribir_en_memoria(void* contenido, uint32_t direccion_fisica, uint32_t bytes_a_escribir);
static void escribir_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t bytes_a_escribir, uint32_t direccion_fisica);
static void solicitar_contenido_a_memoria(uint32_t cantidad_bytes, uint32_t direccion_fisica);
static void* obtener_contenido_a_escribir(uint32_t cantidad_bytes);
static void reposicionamiento_del_puntero_de_archivo(uint32_t puntero_archivo, char *nombre_archivo);


void main_dialfs(t_interfaz *interfaz_hilo)
{
    char *nombre = interfaz_hilo->nombre_interfaz;
    t_config *config = interfaz_hilo->config_interfaz;

    char path[70] = "/home/utnso/tp-2024-1c-SegmenFault/entradasalida/cfg/";

    strcat(path, nombre);
    strcat(path, ".log");

    dialfs_logger = log_create(path, nombre, 1, LOG_LEVEL_INFO); 

    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    path_dial_fs = config_get_string_value(config, "PATH_BASE_DIALFS");
    tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    tamanio_bloque = config_get_int_value(config, "BLOCK_SIZE");
    cantidad_bloques = config_get_int_value(config, "BLOCK_COUNT");
    retraso_compactacion = config_get_int_value(config, "RETRASO_COMPACTACION");

    log_info(dialfs_logger, "Iniciando interfaz DIALFS: %s", nombre);

    tamanio_archivo_bloques = tamanio_bloque * cantidad_bloques;

    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);

    conectarse_a_kernel(socket_kernel, INTERFAZ_DIALFS, nombre, "DIALFS");

    crear_archivo_de_bloque(tamanio_archivo_bloques);

    cargar_bitmap(cantidad_bloques);

    recibir_peticiones_de_kernel();
}

static void recibir_peticiones_de_kernel()
{
    while (1)
    {
        t_paquete *paquete = recibir_paquete(socket_kernel);
        void *stream = paquete->buffer->stream;
        int proceso_conectado = -1;
        char* nombre = NULL;
        uint32_t puntero_archivo = 0;
        uint32_t tamanio = 0;
        uint32_t direccion_fisica = 0;
        uint32_t tamanio_archivo = 0;

        consumir_una_unidad_de_tiempo_de_trabajo();

        switch (paquete->codigo_operacion)
        {
        case CREAR_ARCHIVO:
            nombre = sacar_cadena_de_paquete(&stream);
            proceso_conectado = sacar_entero_de_paquete(&stream);
            log_info(dialfs_logger, "PID: %d - Crear Archivo: %s \n", proceso_conectado, nombre);
            crear_archivo(nombre);
            break;
        case ELIMINAR_ARCHIVO:
            nombre = sacar_cadena_de_paquete(&stream);
            proceso_conectado = sacar_entero_de_paquete(&stream);
            log_info(dialfs_logger, "PID: %d - Eliminar Archivo: %s \n", proceso_conectado, nombre);
            eliminar_archivo(nombre);
            break;
        case TRUNCAR_ARCHIVO:
            nombre = sacar_cadena_de_paquete(&stream);
            tamanio_archivo = sacar_entero_sin_signo_de_paquete(&stream);
            proceso_conectado = sacar_entero_de_paquete(&stream);
            log_info(dialfs_logger, "PID: %d - Truncar Archivo: %s, Tamaño: %d \n", proceso_conectado, nombre, tamanio_archivo);
            truncar_archivo(nombre, tamanio_archivo);
            break;
        case LEER_ARCHIVO:
            nombre = sacar_cadena_de_paquete(&stream);
            puntero_archivo = sacar_entero_sin_signo_de_paquete(&stream);
            tamanio = sacar_entero_sin_signo_de_paquete(&stream);
            proceso_conectado = sacar_entero_de_paquete(&stream);
            direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
            log_info(dialfs_logger, "PID: %d - Leer Archivo: %s - Tamaño a leer : %d - Puntero archivo: %d \n", proceso_conectado, nombre, puntero_archivo, tamanio);
            leer_archivo(nombre, puntero_archivo, tamanio, direccion_fisica);
            break;
        case ESCRIBIR_ARCHIVO:
            nombre = sacar_cadena_de_paquete(&stream);
            puntero_archivo = sacar_entero_sin_signo_de_paquete(&stream);
            tamanio = sacar_entero_sin_signo_de_paquete(&stream);
            proceso_conectado = sacar_entero_de_paquete(&stream);
            direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
            log_info(dialfs_logger, "PID: %d - Escribir Archivo: %s - Tamaño a escribir : %d - Puntero archivo: %d \n", proceso_conectado, nombre, puntero_archivo, tamanio);
            escribir_archivo(nombre, puntero_archivo, tamanio, direccion_fisica);
            break;
        default:
            break;
        }

        eliminar_paquete(paquete);

        aviso_de_operacion_finalizada_a_kernel(proceso_conectado);
    }
}

static void consumir_una_unidad_de_tiempo_de_trabajo()
{
    usleep(tiempo_unidad_trabajo * 1000);
}

static void aviso_de_operacion_finalizada_a_kernel(int proceso_conectado)
{
    send(socket_kernel, &proceso_conectado, sizeof(int), 0);
}

static void crear_archivo(char *nombre_archivo)
{
    char* bloque_inicial = NULL;
    uint32_t buffer_bloque_inicial = buscar_bloque_inicial_libre();

    char *path_archivo = string_from_format("%s/%s", path_dial_fs, nombre_archivo);

    // Creamos el archivo vacio
    FILE *archivo = fopen(path_archivo, "w"); 
    fclose(archivo);

    // Creamos la config del archivo nuevo
    t_config *archivo_nuevo = config_create(path_archivo);

    //Agregamos los valores
    if (archivo_nuevo != NULL)
    {
        config_set_value(archivo_nuevo, "NOMBRE_ARCHIVO", nombre_archivo);

        bloque_inicial = string_from_format("%d", buffer_bloque_inicial);
	    config_set_value(archivo_nuevo, "BLOQUE_INICIAL", bloque_inicial);
        free(bloque_inicial);

        config_set_value(archivo_nuevo, "TAMANIO_ARCHIVO", "0");

        config_save_in_file(archivo_nuevo, path_archivo);

        agregar_bloques(1, buffer_bloque_inicial);

        config_destroy(archivo_nuevo);
        free(path_archivo);
        free(nombre_archivo);
    }
}

static void eliminar_archivo(char *nombre_archivo)
{
    // Obtenemos el path del archivo a eliminar
    metadata_archivo* metadata = levantar_metadata(nombre_archivo);
    uint32_t bloque_inicial = metadata->bloque_inicial;
    uint32_t tamanio_archivo = metadata->tamanio_archivo;
    
    free(metadata);

    // Liberamos el bloque
    uint32_t cantidad_bloques_ocupados = ceil(tamanio_archivo / tamanio_bloque); 
    eliminar_bloques(cantidad_bloques_ocupados, bloque_inicial);

    if(tamanio_archivo == 0){
    eliminar_bloques(1, bloque_inicial);
    }

    //eliminamos el archivo
    char *path_archivo = string_from_format("%s/%s", path_dial_fs, nombre_archivo);
    remove(path_archivo);

    // Liberamos la memoria
    free(path_archivo);
    free(nombre_archivo);
}

static void truncar_archivo(char *nombre_archivo, uint32_t tamanio_nuevo)
{
    // Obtenemos de la metadata los valores inciales
    metadata_archivo* metadata = levantar_metadata(nombre_archivo);
    uint32_t bloque_inicial = metadata->bloque_inicial;
    uint32_t tamanio_actual = metadata->tamanio_archivo;
    free(metadata);

    if (tamanio_nuevo > tamanio_actual)
    {
       ampliar_archivo(tamanio_nuevo, tamanio_actual, bloque_inicial);

       cargamos_cambios_a_metadata_ampliar(tamanio_nuevo, bloque_inicial, nombre_archivo);

    } else if (tamanio_nuevo < tamanio_actual)
    {
        reducir_archivo(tamanio_nuevo, tamanio_actual, bloque_inicial);

        cargamos_cambios_a_metadata_reducir(tamanio_nuevo, nombre_archivo);
    } else
    {
        //No hay que hacer nada
    }

    free(nombre_archivo);
    
}

static void ampliar_archivo(uint32_t tamanio_nuevo, uint32_t tamanio_actual, uint32_t bloque_inicial)
{
   uint32_t bloques_a_agregar = ceil((tamanio_nuevo - tamanio_actual) / tamanio_bloque);
    uint32_t bloque_final_archivo = bloque_inicial + ceil(tamanio_actual / tamanio_bloque);

   //busco si hay la cantidad de bloques contiguos que me piden y si hay los ocupo
   if(!bloques_contiguos(bloques_a_agregar, bloque_final_archivo)) {

        //si no hay compacto
        compactar(bloques_a_agregar, bloque_final_archivo);
        usleep(1000 * retraso_compactacion);
        
   } else printf ("Se encontraron bloques contiguos suficientes \n");

   //Ampliamos el archivo
    agregar_bloques(bloques_a_agregar, bloque_inicial);
   
}

static void reducir_archivo(uint32_t tamanio_nuevo, uint32_t tamanio_actual, uint32_t bloque_inicial)
{
    uint32_t bloques_a_eliminar = ceil((tamanio_actual - tamanio_nuevo) / tamanio_bloque);

    uint32_t primer_bloque_a_borrar = bloque_inicial + ceil(tamanio_nuevo / tamanio_bloque);

    //Eliminamos los bloques
    eliminar_bloques(bloques_a_eliminar, primer_bloque_a_borrar);
}

static void leer_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t bytes_a_leer, uint32_t direccion_fisica)
{
    //Inicializamos el buffer
    void *contenido = malloc(bytes_a_leer);
    
    reposicionamiento_del_puntero_de_archivo(puntero_archivo, nombre_archivo);

    //Leemos el bloque
    fread(contenido, bytes_a_leer, 1, archivo_de_bloques); 
    fclose(archivo_de_bloques);

    //Escribimos el contenido en la memoria
    escribir_en_memoria(contenido, direccion_fisica, bytes_a_leer);
}

static void escribir_en_memoria(void* contenido, uint32_t direccion_fisica, uint32_t bytes_a_escribir)
{
    t_paquete* paquete = crear_paquete(ESCRIBIR_CONTENIDO_EN_MEMORIA);
    agregar_entero_sin_signo_a_paquete(paquete, bytes_a_escribir);
    agregar_bytes_a_paquete(paquete, contenido, bytes_a_escribir);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    enviar_paquete(paquete, socket_memoria);
    free(contenido);
}

static void escribir_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t cantidad_bytes, uint32_t direccion_fisica)
{
    //uint32_t cantidad_bloques = puntero_archivo + ceil(cantidad_bytes / tamanio_bloque);
    
    solicitar_contenido_a_memoria(cantidad_bytes, direccion_fisica);

    void* contenido = obtener_contenido_a_escribir(cantidad_bytes);

    reposicionamiento_del_puntero_de_archivo(puntero_archivo, nombre_archivo);
    
	//Escribimos en el bloque
	fwrite(contenido, cantidad_bytes, 1, archivo_de_bloques);
	fclose(archivo_de_bloques);

	free(contenido);
}

static void solicitar_contenido_a_memoria(uint32_t cantidad_bytes, uint32_t direccion_fisica)
{
	t_paquete* paquete = crear_paquete(LEER_CONTENIDO_EN_MEMORIA);
	agregar_entero_sin_signo_a_paquete(paquete, cantidad_bytes);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
	enviar_paquete(paquete, socket_memoria);
}

static void* obtener_contenido_a_escribir(uint32_t cantidad_bytes)
{
    t_paquete* paquete = recibir_paquete(socket_memoria);
    void* stream = paquete->buffer->stream;

    if (paquete->codigo_operacion == DEVOLVER_LECTURA)
    {
        void* contenido = sacar_bytes_de_paquete(&stream, cantidad_bytes);
        return contenido;
    } else
    {
        perror("Error al obtener el contenido a escribir");
        return NULL;
    }
    eliminar_paquete(paquete);
}
   


static void reposicionamiento_del_puntero_de_archivo(uint32_t puntero_archivo, char *nombre_archivo)
{
    // Obtenemos de la metadata los valores inciales
    metadata_archivo* metadata = levantar_metadata(nombre_archivo);
    uint32_t bloque_inicial = metadata->bloque_inicial;
    uint32_t tamanio_archivo = metadata->tamanio_archivo;
    free(metadata);
    free(nombre_archivo);

    //direccion ultimo bloque a leer
	uint32_t direccion_bloque = tamanio_bloque * bloque_inicial;

	//Obtemos el offset
	uint32_t offset = direccion_bloque + puntero_archivo;
	
	//Abrimos archivo
	archivo_de_bloques = levantar_archivo_bloque();

	//Nos posicionamos en el dato
	fseek(archivo_de_bloques, offset, SEEK_SET);

}
