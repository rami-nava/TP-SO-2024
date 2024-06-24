
#include "io.h"

static char *ip_kernel;
static char *puerto_kernel;
static int socket_kernel;
static int tiempo_unidad_de_trabajo;
t_log* generica_logger;
static char* nombre_interfaz;
static pthread_t hilo_sleep;
static t_list* peticiones;
static sem_t hay_peticiones;
static pthread_mutex_t mutex_lista_peticiones;

static void realizar_sleep();
static void recibir_peticion();

void main_generica(t_interfaz* interfaz){

    nombre_interfaz = interfaz->nombre_interfaz;
    t_config* config_interfaz = interfaz->config_interfaz;

    char path[70] = "/home/utnso/tp-2024-1c-SegmenFault/entradasalida/logs/";

    strcat(path, nombre_interfaz);
    strcat(path, ".log");

    generica_logger = log_create(path, nombre_interfaz, 1, LOG_LEVEL_INFO);

    ip_kernel = config_get_string_value(config_interfaz, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config_interfaz, "PUERTO_KERNEL");
    tiempo_unidad_de_trabajo = config_get_int_value(config_interfaz, "TIEMPO_UNIDAD_TRABAJO");

    log_info(generica_logger, "Iniciando interfaz generica: %s\n", nombre_interfaz);

    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);

    conectarse_a_kernel(socket_kernel, INTERFAZ_GENERICA, nombre_interfaz, "GENERICA");

    sem_init(&hay_peticiones, 0, 0);
    pthread_mutex_init(&mutex_lista_peticiones, NULL);
    
    recibir_peticion();
}

void recibir_peticion() 
{
    peticiones = list_create();

    pthread_create(&hilo_sleep, NULL, (void* ) realizar_sleep, NULL);
    pthread_detach(hilo_sleep);
    
    while(1)
    {
        t_paquete* paquete = recibir_paquete(socket_kernel);

        void* stream = paquete->buffer->stream;

        if(paquete->codigo_operacion == GENERICA_IO_SLEEP) {

        t_peticion_generica* peticion = malloc(sizeof(t_peticion_generica));
        
        peticion->pid = sacar_entero_de_paquete(&stream);
        peticion->tiempo_sleep = sacar_entero_de_paquete(&stream);

        pthread_mutex_lock(&mutex_lista_peticiones);
        list_add(peticiones, peticion);
        pthread_mutex_unlock(&mutex_lista_peticiones);

        sem_post(&hay_peticiones);

        }
        eliminar_paquete(paquete);
    }
}
//TODO AGREGAR MUTEX A LA LISTA

void realizar_sleep(){

    while(true){
        sem_wait(&hay_peticiones);

        pthread_mutex_lock(&mutex_lista_peticiones);
        t_peticion_generica* peticion = list_remove(peticiones, 0);
        pthread_mutex_unlock(&mutex_lista_peticiones);
        
        log_info(generica_logger, "PID: %d - Operacion: IO_GEN_SLEEP\n", peticion->pid);

        int tiempo_sleep = tiempo_unidad_de_trabajo * peticion->tiempo_sleep * 1000;

        usleep(tiempo_sleep);

        log_info(generica_logger, "El proceso finalizo IO\n");

        send(socket_kernel, &peticion->pid, sizeof(int), 0);

        free(peticion);
    }
}

void desconectar_generica(){
    int desconexion = -1;
    send(socket_kernel, &desconexion, sizeof(int), 0);

    int desconectado_kernel = 0;
    recv(socket_kernel, &desconectado_kernel , sizeof(int), MSG_WAITALL);
}