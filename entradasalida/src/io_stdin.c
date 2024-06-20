#include "io.h"

// Funciones Locales //
static void guardar_escritura();
static void solicitar_escritura(void* texto_a_guardar, t_list* direcciones_fisicas, int pid);
static void pedido_escritura(void* contenido, uint32_t direccion_fisica, uint32_t bytes_a_escribir, int pid);
static void recibir_peticion();

static char *ip_kernel;
static char *puerto_kernel;
static char *ip_memoria;
static char *puerto_memoria;
static int socket_kernel;
static int socket_memoria;
static t_log* stdin_logger;
static pthread_t hilo_stdin;
static t_list* peticiones;
static sem_t hay_peticiones;
static pthread_mutex_t mutex_lista_peticiones;

void main_stdin(t_interfaz* interfaz_hilo) 
{    
    char* nombre = interfaz_hilo->nombre_interfaz;
    t_config* config = interfaz_hilo->config_interfaz;

    char path[70] = "/home/utnso/tp-2024-1c-SegmenFault/entradasalida/logs/";

    strcat(path, nombre);
    strcat(path, ".log");

    stdin_logger = log_create(path, nombre, 1, LOG_LEVEL_INFO);

    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    
    log_info(stdin_logger, "Iniciando interfaz STDIN: %s\n", nombre);
    
    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);

    conectarse_a_kernel(socket_kernel, INTERFAZ_STDIN, nombre, "STDIN");

    sem_init(&hay_peticiones, 0, 0);
    pthread_mutex_init(&mutex_lista_peticiones, NULL);
    
    recibir_peticion();
}

static void recibir_peticion() 
{
    peticiones = list_create();

    pthread_create(&hilo_stdin, NULL, (void* ) guardar_escritura, NULL);
    pthread_detach(hilo_stdin);

    while (1)
    {
        t_paquete* paquete = recibir_paquete(socket_kernel);
        void* stream = paquete->buffer->stream;

        if(paquete->codigo_operacion == STDIN_READ)
        {
            t_peticion_std* peticion = malloc(sizeof(t_peticion_std));

            peticion->pid= sacar_entero_de_paquete(&stream);
            peticion->direcciones_fisicas = sacar_lista_de_accesos_de_paquete(&stream);
            peticion->tamanio_registro = sacar_entero_sin_signo_de_paquete(&stream);

            pthread_mutex_lock(&mutex_lista_peticiones);
            list_add(peticiones, peticion);
            pthread_mutex_unlock(&mutex_lista_peticiones);

            sem_post(&hay_peticiones);
        } 
        else{
            log_error(stdin_logger, "Se recibio una peticion invalida: %d\n", paquete->codigo_operacion);
            abort();
        }
        
        eliminar_paquete(paquete);
    }
}

static void guardar_escritura() 
{
    while(true){
        sem_wait(&hay_peticiones);

        pthread_mutex_lock(&mutex_lista_peticiones);
        t_peticion_std* peticion = list_remove(peticiones, 0);
        pthread_mutex_unlock(&mutex_lista_peticiones);

        log_info(stdin_logger, "PID: %d - Operacion: IO_STDIN_READ\n", peticion->pid);

        char* leer_linea;
        char texto_a_guardar[peticion->tamanio_registro];

        leer_linea = readline("Ingrese el texto que desea guardar en memoria > \n");
        
        if(leer_linea){
            //Solo se va a guardar dependiendo de la cantidad especificada en el parametro tamanio
            strncpy(texto_a_guardar, leer_linea, peticion->tamanio_registro); 
            texto_a_guardar[peticion->tamanio_registro] = '\0';
            free(leer_linea);

            log_info(stdin_logger, "Guardando texto en memoria: %s\n", texto_a_guardar);
            
            //IO solicita que memoria guarde el texto en la direccion especificada
            solicitar_escritura(texto_a_guardar, peticion->direcciones_fisicas, peticion->pid);

            //Le avisa a kernel que el texto fue guardado en memoria
            send(socket_kernel, &peticion->pid, sizeof(int), 0);

            free(peticion);
        }
    }
}

static void solicitar_escritura(void* texto_a_guardar, t_list* direcciones_fisicas, int pid)
{
    uint32_t escritura_guardada_stdin;
    uint32_t tamanio_copiado_actual = 0;

    t_list_iterator* iterator = list_iterator_create(direcciones_fisicas);

    // Mientras haya una df a escribir, sigo escribiendo
    while (list_iterator_has_next(iterator)) {

        // Obtengo el próximo acceso de la lista
        t_acceso_memoria* acceso = (t_acceso_memoria*) list_iterator_next(iterator);
        
        // Buffer donde se va a copiar el contenido de a poco
        void* contenido_a_escribir_actual = malloc(acceso->tamanio);

        // Copio el contenido en el buffer
        memcpy(contenido_a_escribir_actual, texto_a_guardar + tamanio_copiado_actual, acceso->tamanio);

        // Lo escribo en memoria
        pedido_escritura(contenido_a_escribir_actual, acceso->direccion_fisica, acceso->tamanio, pid);
        recv(socket_memoria, &escritura_guardada_stdin, sizeof(uint32_t), MSG_WAITALL);

        if (escritura_guardada_stdin == 1){
            tamanio_copiado_actual += acceso->tamanio; 
        }else{
            log_error(stdin_logger, "Escritura fallida\n");
            abort();
        }        
    }
    list_iterator_destroy(iterator);
    
    list_destroy_and_destroy_elements(direcciones_fisicas, free);
}

static void pedido_escritura(void* contenido, uint32_t direccion_fisica, uint32_t bytes_a_escribir, int pid){
    t_paquete* paquete = crear_paquete(ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_STDIN);
    agregar_entero_a_paquete(paquete, pid);
    agregar_entero_sin_signo_a_paquete(paquete, bytes_a_escribir);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_bytes_a_paquete(paquete, contenido, bytes_a_escribir);
    enviar_paquete(paquete, socket_memoria);
    free(contenido);
}

void desconectar_stdin(){
    desconectar_memoria_stdin();

    int desconexion = -1;
    send(socket_kernel, &desconexion, sizeof(int), 0);

    int desconectado_kernel = 0;
    recv(socket_kernel, &desconectado_kernel , sizeof(int), MSG_WAITALL);
}

void desconectar_memoria_stdin(){
    t_paquete* paquete = crear_paquete(DESCONECTAR_IO);
    agregar_entero_a_paquete(paquete,1);
    enviar_paquete(paquete, socket_memoria);
}