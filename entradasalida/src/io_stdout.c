#include "io.h"

// Variables Locales //
static char *ip_kernel;
static char *puerto_kernel;
static char *ip_memoria;
static char *puerto_memoria;
static int socket_kernel;
static int socket_memoria;
t_log* stdout_logger;
static pthread_t hilo_stdout;
static t_list* peticiones;
static sem_t hay_peticiones;
static pthread_mutex_t mutex_lista_peticiones;

// Funciones Locales //
static void recibir_peticion();
static char* pedir_lectura_a_memoria(t_list* direcciones_fisicas, uint32_t tamanio, int pid); 
static void peticion_de_lectura(uint32_t direccion_fisica, uint32_t tamanio, int pid);
static void* recibir_lectura();
static void leer_memoria(); 

void main_stdout(t_interfaz* interfaz_hilo) 
{
    char* nombre = interfaz_hilo->nombre_interfaz;
    t_config* config = interfaz_hilo->config_interfaz;

    char path[70] = "/home/utnso/tp-2024-1c-SegmenFault/entradasalida/logs/";

    strcat(path, nombre);
    strcat(path, ".log");

    stdout_logger = log_create(path, nombre, 1, LOG_LEVEL_INFO);
    
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    
    log_info(stdout_logger, "Iniciando interfaz STDOUT: %s", nombre);
        
    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);

    conectarse_a_kernel(socket_kernel, INTERFAZ_STDOUT, nombre, "STDOUT");

    sem_init(&hay_peticiones, 0, 0);
    pthread_mutex_init(&mutex_lista_peticiones, NULL);
    
    recibir_peticion();

}   

static void recibir_peticion ()
{
    peticiones = list_create();

    pthread_create(&hilo_stdout, NULL, (void* ) leer_memoria, NULL);
    pthread_detach(hilo_stdout);

    while (1)
    {
        t_paquete* paquete = recibir_paquete(socket_kernel);
        void* stream = paquete->buffer->stream;

        if(paquete->codigo_operacion == STDOUT_WRITE)
        {
            t_peticion_std* peticion = malloc(sizeof(t_peticion_std));

            peticion->pid = sacar_entero_de_paquete(&stream);
            peticion->direcciones_fisicas = sacar_lista_de_accesos_de_paquete(&stream);
            peticion->tamanio_registro = sacar_entero_sin_signo_de_paquete(&stream);

            pthread_mutex_lock(&mutex_lista_peticiones);
            list_add(peticiones, peticion);
            pthread_mutex_unlock(&mutex_lista_peticiones);

            sem_post(&hay_peticiones);
        } 
        else {
            log_error(stdout_logger, "Se recibio un paquete erroneo: %d", paquete->codigo_operacion);
            abort();
        }
        eliminar_paquete(paquete);
    }
}

static void leer_memoria(){
   
   while(true){
        sem_wait(&hay_peticiones);

        pthread_mutex_lock(&mutex_lista_peticiones);
        t_peticion_std* peticion = list_remove(peticiones, 0);
        pthread_mutex_unlock(&mutex_lista_peticiones);

        log_info(stdout_logger, "PID: %d - Operacion: IO_STDOUT_WRITE\n", peticion->pid);

        //Le pide la lectura de esa direccion/es a la memoria 
        char* lectura = pedir_lectura_a_memoria(peticion->direcciones_fisicas, peticion->tamanio_registro, peticion->pid);

        //Mostramos por pantalla la lectura
        printf("Lectura realizada: %s\n", lectura);

        free(lectura);
                
        //Le avisa a Kernel que ya se realizo la lectura, y ya se mostro por pantalla
        send(socket_kernel, &peticion->pid, sizeof(int), 0);
        free(peticion);
   }
}

static char* pedir_lectura_a_memoria(t_list* direcciones_fisicas, uint32_t tamanio_lectura, int pid) 
{
    uint32_t tamanio_leido_actual = 0;

    // Buffer donde se va a ir almacenando el contenido leído final
    char* lectura = malloc(tamanio_lectura); 

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

static void peticion_de_lectura(uint32_t direccion_fisica, uint32_t tamanio, int pid)
{
    t_paquete* paquete = crear_paquete(LEER_CONTENIDO_EN_MEMORIA_DESDE_STDOUT);
    agregar_entero_a_paquete(paquete, pid);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    enviar_paquete(paquete, socket_memoria);
}

static void* recibir_lectura() 
{
    t_paquete* paquete = recibir_paquete(socket_memoria);
    void* stream = paquete->buffer->stream;

    if(paquete->codigo_operacion == RESULTADO_LECTURA_STDOUT){
        void* lectura = sacar_bytes_de_paquete(&stream);

        eliminar_paquete(paquete);
        return lectura;
    }
    else {
        log_error(stdout_logger, "No me enviaste el contenido \n");
        eliminar_paquete(paquete);
        abort();
    }
}

void desconectar_stdout(){
    desconectar_memoria_stdin();

    int desconexion = -1;
    send(socket_kernel, &desconexion, sizeof(int), 0);

    int desconectado_kernel = 0;
    recv(socket_kernel, &desconectado_kernel , sizeof(int), MSG_WAITALL);
}

void desconectar_memoria_stdout(){
    t_paquete* paquete = crear_paquete(DESCONECTAR_IO);
    agregar_entero_a_paquete(paquete,1);
    enviar_paquete(paquete, socket_memoria);
}