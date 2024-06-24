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
t_list* bloques_iniciales;
t_dictionary* nombre_con_bloque_inicial;
bool compactar_desde_comienzo;
static pthread_t hilo_dialfs;
static t_list* peticiones;
static sem_t hay_peticiones;
static pthread_mutex_t mutex_lista_peticiones;

static void recibir_peticion();
static void consumir_una_unidad_de_tiempo_de_trabajo();
static void crear_archivo(char *nombre_archivo);
static void eliminar_archivo(char *nombre_archivo);
static void truncar_archivo(char *nombre_archivo, uint32_t tamanio_nuevo, int pid);
static void ampliar_archivo(uint32_t tamanio_nuevo, uint32_t tamanio_actual, uint32_t bloque_inicial, int pid);
static void reducir_archivo(uint32_t tamanio_nuevo, uint32_t tamanio_actual, uint32_t bloque_inicial);
static void leer_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t bytes_a_leer, t_list* lista_accesos_memoria, int pid);
static void pedido_escritura(void* contenido_a_escribir, uint32_t direccion_fisica, uint32_t tamanio, int pid);
static void escribir_en_memoria(void* contenido, t_list* direcciones_fisicas, uint32_t bytes_a_escribir, int pid);
static void escribir_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t bytes_a_escribir, t_list* lista_accesos_memoria, int pid);
static void peticion_de_lectura(uint32_t direccion_fisica, uint32_t cantidad_bytes, int pid);
static void* pedir_lectura_a_memoria(t_list* direcciones_fisicas, uint32_t tamanio_lectura, int pid);
static void* recibir_lectura();
static void reposicionamiento_del_puntero_de_archivo(uint32_t puntero_archivo, char *nombre_archivo);
static void atender_peticion();


void main_dialfs(t_interfaz *interfaz_hilo)
{
    char *nombre = interfaz_hilo->nombre_interfaz;
    t_config *config = interfaz_hilo->config_interfaz;
    bloques_iniciales = list_create();
    nombre_con_bloque_inicial = dictionary_create();

    char path[70] = "/home/utnso/tp-2024-1c-SegmenFault/entradasalida/logs/";

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

    sem_init(&hay_peticiones, 0, 0);
    pthread_mutex_init(&mutex_lista_peticiones, NULL);

    recibir_peticion();
}

static void recibir_peticion()
{
    peticiones = list_create();
    
    pthread_create(&hilo_dialfs, NULL, (void* ) atender_peticion, NULL);
    pthread_detach(hilo_dialfs);

    while (1)
    {
        t_paquete *paquete = recibir_paquete(socket_kernel);
        void *stream = paquete->buffer->stream;

        consumir_una_unidad_de_tiempo_de_trabajo();

        t_peticion_dialfs* peticion = malloc(sizeof(t_peticion_dialfs));
        peticion->op_code = paquete->codigo_operacion;

        switch (paquete->codigo_operacion)
        {
        case CREAR_ARCHIVO:         
            peticion->nombre = sacar_cadena_de_paquete(&stream);
            peticion->pid = sacar_entero_de_paquete(&stream);
            log_info(dialfs_logger, "PID: %d - Crear Archivo: %s \n", peticion->pid, peticion->nombre);
            pthread_mutex_lock(&mutex_lista_peticiones);
            list_add(peticiones, peticion);
            pthread_mutex_unlock(&mutex_lista_peticiones);
            sem_post(&hay_peticiones);
            break;
        case ELIMINAR_ARCHIVO:
            peticion->nombre = sacar_cadena_de_paquete(&stream);
            peticion->pid = sacar_entero_de_paquete(&stream);
            log_info(dialfs_logger, "PID: %d - Eliminar Archivo: %s \n", peticion->pid, peticion->nombre);
            pthread_mutex_lock(&mutex_lista_peticiones);
            list_add(peticiones, peticion);
            pthread_mutex_unlock(&mutex_lista_peticiones);
            sem_post(&hay_peticiones);
            break;
        case TRUNCAR_ARCHIVO:
            peticion->nombre = sacar_cadena_de_paquete(&stream);
            peticion->tamanio_archivo = sacar_entero_sin_signo_de_paquete(&stream);
            peticion->pid = sacar_entero_de_paquete(&stream);
            log_info(dialfs_logger, "PID: %d - Truncar Archivo: %s, Tamaño: %d \n", peticion->pid, peticion->nombre, peticion->tamanio_archivo);
            pthread_mutex_lock(&mutex_lista_peticiones);
            list_add(peticiones, peticion);
            pthread_mutex_unlock(&mutex_lista_peticiones);
            sem_post(&hay_peticiones);
            break;
        case LEER_ARCHIVO:
            peticion->nombre = sacar_cadena_de_paquete(&stream);
            peticion->puntero_archivo = sacar_entero_sin_signo_de_paquete(&stream);
            peticion->tamanio = sacar_entero_sin_signo_de_paquete(&stream);
            peticion->pid = sacar_entero_de_paquete(&stream);
            peticion->direcciones_fisicas = sacar_lista_de_accesos_de_paquete(&stream);
            log_info(dialfs_logger, "PID: %d - Leer Archivo: %s - Tamaño a leer : %d - Puntero archivo: %d \n", peticion->pid, peticion->nombre, peticion->puntero_archivo, peticion->tamanio);
            pthread_mutex_lock(&mutex_lista_peticiones);
            list_add(peticiones, peticion);
            pthread_mutex_unlock(&mutex_lista_peticiones);
            sem_post(&hay_peticiones);
            break;
        case ESCRIBIR_ARCHIVO:
            peticion->nombre = sacar_cadena_de_paquete(&stream);
            peticion->puntero_archivo = sacar_entero_sin_signo_de_paquete(&stream);
            peticion->tamanio = sacar_entero_sin_signo_de_paquete(&stream);
            peticion->pid = sacar_entero_de_paquete(&stream);
            peticion->direcciones_fisicas = sacar_lista_de_accesos_de_paquete(&stream);

            log_info(dialfs_logger, "PID: %d - Escribir Archivo: %s - Tamaño a escribir : %d - Puntero archivo: %d \n", peticion->pid, peticion->nombre, peticion->tamanio, peticion->puntero_archivo);
            pthread_mutex_lock(&mutex_lista_peticiones);
            list_add(peticiones, peticion);
            pthread_mutex_unlock(&mutex_lista_peticiones);
            sem_post(&hay_peticiones);
            break;
        case LEER_BITMAP:
            peticion->desde = sacar_entero_de_paquete(&stream);
            peticion->hasta = sacar_entero_de_paquete(&stream);
            pthread_mutex_lock(&mutex_lista_peticiones);
            list_add(peticiones, peticion);
            pthread_mutex_unlock(&mutex_lista_peticiones);
            sem_post(&hay_peticiones);
            break;
        default:
            break;
        }

        eliminar_paquete(paquete);
    }
}

static void atender_peticion(){
    while(true){
        sem_wait(&hay_peticiones);
        pthread_mutex_lock(&mutex_lista_peticiones);
        t_peticion_dialfs* peticion = list_remove(peticiones, 0);
        pthread_mutex_unlock(&mutex_lista_peticiones);

        switch(peticion->op_code){
            case CREAR_ARCHIVO:
                crear_archivo(peticion->nombre);
                break;
            case ELIMINAR_ARCHIVO:
                eliminar_archivo(peticion->nombre);
                break;
            case TRUNCAR_ARCHIVO:
                truncar_archivo(peticion->nombre, peticion->tamanio_archivo, peticion->pid);
                break;
            case LEER_ARCHIVO:
                leer_archivo(peticion->nombre, peticion->puntero_archivo, peticion->tamanio, peticion->direcciones_fisicas, peticion->pid);
                break;
            case ESCRIBIR_ARCHIVO:
                escribir_archivo(peticion->nombre, peticion->puntero_archivo, peticion->tamanio, peticion->direcciones_fisicas, peticion->pid);
                break;
            case LEER_BITMAP:
                leer_bitmap(peticion->desde, peticion->hasta);
                break;
            default:
                break;
        }

        send(socket_kernel, &peticion->pid, sizeof(int), 0);

        free(peticion);
    }
}

static void consumir_una_unidad_de_tiempo_de_trabajo()
{
    usleep(tiempo_unidad_trabajo * 1000);
}

static void crear_archivo(char *nombre_archivo)
{
    char* bloque_inicial = NULL;
    char* tamanio_inicial = NULL;

    uint32_t buffer_bloque_inicial = buscar_bloque_inicial_libre();
    
    list_add(bloques_iniciales, (void*)(intptr_t)buffer_bloque_inicial);

    char bloque_inicial_string[20]; //Alcanza para almacenar un uint32_t
    snprintf(bloque_inicial_string, sizeof(bloque_inicial_string), "%u", buffer_bloque_inicial);

    dictionary_put(nombre_con_bloque_inicial, strdup(bloque_inicial_string), strdup(nombre_archivo));

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

        tamanio_inicial = string_from_format("%d", tamanio_bloque);
        config_set_value(archivo_nuevo, "TAMANIO_ARCHIVO", tamanio_inicial);
        free(tamanio_inicial);

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

    //eliminamos el archivo
    char *path_archivo = string_from_format("%s/%s", path_dial_fs, nombre_archivo);
    remove(path_archivo);

    // Liberamos la memoria
    free(path_archivo);
    free(nombre_archivo);
}

static void truncar_archivo(char *nombre_archivo, uint32_t tamanio_nuevo, int pid)
{
    // Obtenemos de la metadata los valores inciales
    metadata_archivo* metadata = levantar_metadata(nombre_archivo);
    uint32_t bloque_inicial = metadata->bloque_inicial;
    uint32_t tamanio_actual = metadata->tamanio_archivo;
    free(metadata);

    if (tamanio_nuevo > tamanio_actual)
    {
       ampliar_archivo(tamanio_nuevo, tamanio_actual, bloque_inicial, pid);

       cargamos_cambios_a_metadata_ampliar(tamanio_nuevo, nombre_archivo);

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

static void ampliar_archivo(uint32_t tamanio_nuevo, uint32_t tamanio_actual, uint32_t bloque_inicial, int pid)
{
    compactar_desde_comienzo = false;
    
    uint32_t bloques_a_agregar = ceil((tamanio_nuevo - tamanio_actual) / tamanio_bloque);
    uint32_t cantidad_bloques_del_archivo = ceil(tamanio_actual / tamanio_bloque);

    uint32_t bloque_final_archivo = bloque_inicial + cantidad_bloques_del_archivo - 1;

    //Comprobamos si hay suficientes bloques contiguos
   if(!bloques_contiguos(bloques_a_agregar, bloque_final_archivo)) {

        log_info(dialfs_logger, "PID: %d - Inicio Compactación \n", pid);

        compactar(bloques_a_agregar, bloque_final_archivo);

        //Si no hay bloques libres suficientes a la derecha compacto
        if(compactar_desde_comienzo) {
            //Muevo todos los bloques libres a la izquierda del archivo, a su derecha
            //El bloque final del archivo cambia, entonces lo actualizo
            bloque_final_archivo = compactar_desde_el_comienzo(bloque_final_archivo);
        }

        //La llamo denuevo, esta vez ya deberia tener bloques libres a la derecha para asignarle al archivo
        compactar(bloques_a_agregar, bloque_final_archivo);

        usleep(1000 * retraso_compactacion);

        log_info(dialfs_logger, "PID: %d - Fin Compactación \n", pid);
        
   } else printf ("Se encontraron bloques contiguos suficientes \n");

   //Ampliamos el archivo
    agregar_bloques(bloques_a_agregar, bloque_final_archivo);
   
}

static void reducir_archivo(uint32_t tamanio_nuevo, uint32_t tamanio_actual, uint32_t bloque_inicial)
{
    uint32_t bloques_a_eliminar = ceil((tamanio_actual - tamanio_nuevo) / tamanio_bloque);
    uint32_t cantidad_bloques_del_archivo = ceil(tamanio_actual / tamanio_bloque);

    uint32_t primer_bloque_a_borrar = bloque_inicial + cantidad_bloques_del_archivo;

    //Eliminamos los bloques
    eliminar_bloques(bloques_a_eliminar, primer_bloque_a_borrar);
}

static void leer_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t bytes_a_escribir, t_list* direcciones_fisicas, int pid)
{
    //Inicializamos el buffer
    void *contenido = malloc(bytes_a_escribir);
    
    reposicionamiento_del_puntero_de_archivo(puntero_archivo, nombre_archivo);

    //Leemos el bloque
    fread(contenido, bytes_a_escribir, 1, archivo_de_bloques); 
    fclose(archivo_de_bloques);

    //Escribimos el contenido en la memoria
    escribir_en_memoria(contenido, direcciones_fisicas, bytes_a_escribir, pid);
}

static void escribir_en_memoria(void* contenido, t_list* direcciones_fisicas, uint32_t bytes_a_escribir, int pid)
{
    uint32_t escritura_guardada_dialfs;
    uint32_t tamanio_copiado_actual = 0;

    t_list_iterator* iterator = list_iterator_create(direcciones_fisicas);

    // Mientras haya una df a escribir, sigo escribiendo
    while (list_iterator_has_next(iterator)) {

        // Obtengo el próximo acceso de la lista
        t_acceso_memoria* acceso = (t_acceso_memoria*) list_iterator_next(iterator);
        
        // Buffer donde se va a copiar el contenido de a poco
        void* contenido_a_escribir_actual = malloc(acceso->tamanio);

        // Copio el contenido en el buffer
        memcpy(contenido_a_escribir_actual, contenido + tamanio_copiado_actual, acceso->tamanio);

        // Lo escribo en memoria
        pedido_escritura(contenido_a_escribir_actual, acceso->direccion_fisica, acceso->tamanio, pid);
        recv(socket_memoria, &escritura_guardada_dialfs, sizeof(uint32_t), MSG_WAITALL);

        if (escritura_guardada_dialfs == 1){
            tamanio_copiado_actual += acceso->tamanio; 
        }else{
            log_error(dialfs_logger, "Escritura fallida\n");
            abort();
        }        
    }
    list_iterator_destroy(iterator);
    
    list_destroy_and_destroy_elements(direcciones_fisicas, free);
}

static void pedido_escritura(void* contenido_a_escribir, uint32_t direccion_fisica, uint32_t bytes_a_escribir, int pid)
{
    t_paquete* paquete = crear_paquete(ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_DIALFS);
    agregar_entero_a_paquete(paquete, pid);
    agregar_entero_sin_signo_a_paquete(paquete, bytes_a_escribir);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_bytes_a_paquete(paquete, contenido_a_escribir, bytes_a_escribir);
    enviar_paquete(paquete, socket_memoria);
    free(contenido_a_escribir);
}

static void escribir_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t cantidad_bytes, t_list* direcciones_fisicas, int pid)
{
    //uint32_t cantidad_bloques = puntero_archivo + ceil(cantidad_bytes / tamanio_bloque);
    
    void* contenido = pedir_lectura_a_memoria(direcciones_fisicas, cantidad_bytes, pid);

    reposicionamiento_del_puntero_de_archivo(puntero_archivo, nombre_archivo);
    
	//Escribimos en el bloque
	fwrite(contenido, cantidad_bytes, 1, archivo_de_bloques);
	fclose(archivo_de_bloques);

	free(contenido);
}

static void* pedir_lectura_a_memoria(t_list* direcciones_fisicas, uint32_t tamanio_lectura, int pid) 
{
    uint32_t tamanio_leido_actual = 0;

    // Buffer donde se va a ir almacenando el contenido leído final
    void* lectura = malloc(tamanio_lectura); 

    t_list_iterator* iterator = list_iterator_create(direcciones_fisicas);

     // Mientras haya una df a escribir, sigo escribiendo
    while (list_iterator_has_next(iterator)) {

        // Obtengo el próximo acceso de la lista
        t_acceso_memoria* acceso = (t_acceso_memoria*) list_iterator_next(iterator);
        
        // Leo en Memoria
        peticion_de_lectura(acceso->direccion_fisica, acceso->tamanio, pid);

        // Guardo en un buffer la lectura y lo copio en el contenido total
        void* buffer_lectura = recibir_lectura(); 
        memcpy(lectura + tamanio_leido_actual, buffer_lectura, acceso->tamanio);

        // Actualizo el tamanio leido
        tamanio_leido_actual += acceso->tamanio;

        // Libero la memoria del buffer
        free(buffer_lectura); 
    }

    list_iterator_destroy(iterator);
    
    list_destroy_and_destroy_elements(direcciones_fisicas, free);

    return lectura;
}

static void* recibir_lectura() 
{
    t_paquete* paquete = recibir_paquete(socket_memoria);
    void* stream = paquete->buffer->stream;

    if(paquete->codigo_operacion == VALOR_LECTURA){
        void* lectura = sacar_bytes_de_paquete(&stream);

        eliminar_paquete(paquete);
        return lectura;
    }
    else {
        log_error(dialfs_logger, "No me enviaste el contenido \n");
        eliminar_paquete(paquete);
        abort();
    }
}

static void peticion_de_lectura(uint32_t direccion_fisica, uint32_t cantidad_bytes, int pid)
{
	t_paquete* paquete = crear_paquete(LEER_CONTENIDO_EN_MEMORIA_DIALFS);
    agregar_entero_a_paquete(paquete, pid);
	agregar_entero_sin_signo_a_paquete(paquete, cantidad_bytes);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
	enviar_paquete(paquete, socket_memoria);
}

static void reposicionamiento_del_puntero_de_archivo(uint32_t puntero_archivo, char *nombre_archivo)
{
    // Obtenemos de la metadata los valores inciales
    metadata_archivo* metadata = levantar_metadata(nombre_archivo);
    uint32_t bloque_inicial = metadata->bloque_inicial;
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

void desconectar_dialfs(){
    desconectar_memoria_dialfs();

    int desconexion = -1;
    send(socket_kernel, &desconexion, sizeof(int), 0);

    int desconectado_kernel = 0;
    recv(socket_kernel, &desconectado_kernel , sizeof(int), MSG_WAITALL);
}

void desconectar_memoria_dialfs(){
    t_paquete* paquete = crear_paquete(DESCONECTAR_IO);
    agregar_entero_a_paquete(paquete,1);
    enviar_paquete(paquete, socket_memoria);
}