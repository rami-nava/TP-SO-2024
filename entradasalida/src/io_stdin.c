#include "io.h"

// Funciones Locales //
static void guardar_escritura();
static void solicitar_escritura(void* texto_a_guardar, int pid, uint32_t direccion_fisica, uint32_t tamanio_registro);
static void recibir_peticion();

static char *ip_kernel;
static char *puerto_kernel;
static char *ip_memoria;
static char *puerto_memoria;
static int socket_kernel;
static int socket_memoria;
t_log* stdin_logger;
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
            peticion->direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
            peticion->tamanio_registro = sacar_entero_sin_signo_de_paquete(&stream);

            pthread_mutex_lock(&mutex_lista_peticiones);
            list_add(peticiones, peticion);
            pthread_mutex_unlock(&mutex_lista_peticiones);

            sem_post(&hay_peticiones);
        } 
        else if(paquete->codigo_operacion == FINALIZAR_OPERACION_IO){
            sacar_entero_de_paquete(&stream);
            
            //Enviar paquete para que el hilo de kernel no quede esperando 
            int termino_io = -1;
            send(socket_kernel, &termino_io, sizeof(int), 0);
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
            strncpy(texto_a_guardar, leer_linea, peticion->tamanio_registro); //Solo se va a guardar dependiendo de la cantidad especificada en el parametro
            texto_a_guardar[peticion->tamanio_registro] = '\0';
            free(leer_linea);

            log_info(stdin_logger, "Guardando texto en memoria: %s\n", texto_a_guardar);
            
            //IO solicita que memoria guarde el texto en la direccion especificada
            solicitar_escritura(texto_a_guardar, peticion->pid, peticion->direccion_fisica, peticion->tamanio_registro);
            
            //Memoria confirma que guardo el texto en la direccion especificada
            uint32_t escritura_guardada;
            recv(socket_memoria, &escritura_guardada, sizeof(uint32_t), MSG_WAITALL); 

            //Le avisa a kernel que el texto fue guardado en memoria
            send(socket_kernel, &peticion->pid, sizeof(int), 0);

            free(peticion);
        }
    }
}

/*static void solicitar_escritura(void* texto_a_guardar, int pid) {
    t_paquete* paquete = crear_paquete(PEDIDO_MOV_OUT);
    agregar_entero_a_paquete(paquete, pid);
    agregar_entero_sin_signo_a_paquete(paquete,direccion_fisica);
    agregar_entero_a_paquete(paquete, tamanio_registro);
    agregar_bytes_a_paquete(paquete, texto_a_guardar, tamanio_registro);
    enviar_paquete(paquete, socket_memoria);
}*/

static void solicitar_escritura(void* texto_a_guardar, int pid, uint32_t direccion_fisica, uint32_t tamanio_registro){
    t_paquete* paquete = crear_paquete(ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_STDIN);
    agregar_entero_a_paquete(paquete, pid);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio_registro);
    agregar_entero_sin_signo_a_paquete(paquete,direccion_fisica);
    agregar_bytes_a_paquete(paquete, texto_a_guardar, tamanio_registro);
    enviar_paquete(paquete, socket_memoria);
}

void desconectar_memoria_stdin(){
    t_paquete* paquete = crear_paquete(DESCONECTAR_IO);
    agregar_entero_a_paquete(paquete,1);
    enviar_paquete(paquete, socket_memoria);
}