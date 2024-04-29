
#include "io.h"

static char *ip_kernel;
static char *puerto_kernel;
static int socket_kernel;
static int tiempo_unidad_de_trabajo;
t_log* generica_logger;
static char* nombre_interfaz;
static t_temporal* reloj;
static bool proceso_eliminado;

static void dormir(int duracion);

static void realizar_sleep();

void main_generica(t_interfaz* interfaz){

    nombre_interfaz = interfaz->nombre_interfaz;
    t_config* config_interfaz = interfaz->config_interfaz;

    char path[70] = "/home/utnso/tp-2024-1c-SegmenFault/entradasalida/cfg/";

    strcat(path, nombre_interfaz);
    strcat(path, ".log");

    generica_logger = log_create(path, nombre_interfaz, 1, LOG_LEVEL_INFO);

    ip_kernel = config_get_string_value(config_interfaz, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config_interfaz, "PUERTO_KERNEL");
    tiempo_unidad_de_trabajo = config_get_int_value(config_interfaz, "TIEMPO_UNIDAD_TRABAJO");

    log_info(generica_logger, "Iniciando interfaz generica: %s\n", nombre_interfaz);

    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);

    conectarse_a_kernel(socket_kernel, INTERFAZ_GENERICA, nombre_interfaz, "GENERICA");
    
    realizar_sleep();
}

void realizar_sleep() 
{
    int proceso_conectado;
    int cantidad_tiempo;
    
    while(1)
    {
        t_paquete* paquete = recibir_paquete(socket_kernel);
        void* stream = paquete->buffer->stream;

        if(paquete->codigo_operacion == GENERICA_IO_SLEEP) {

        proceso_conectado = sacar_entero_de_paquete(&stream);
        cantidad_tiempo = sacar_entero_de_paquete(&stream);

        log_info(generica_logger, "PID: %d - Operacion: IO_GEN_SLEEP\n", proceso_conectado);
        
        proceso_eliminado = false;
        int tiempo_sleep = tiempo_unidad_de_trabajo * cantidad_tiempo * 1000;
        pthread_t hilo_sleep;
        pthread_create(&hilo_sleep, NULL, dormir, tiempo_sleep);
        pthread_detach(&hilo_sleep);

        }else if(paquete->codigo_operacion == FINALIZAR_OPERACION_IO){
            sacar_entero_de_paquete(&stream);
            proceso_eliminado = true;
            
            //Enviar paquete para que el hilo de kernel no quede esperando 
            int termino_io = -1;
            send(socket_kernel, &termino_io, sizeof(int), 0);
        }
        
        eliminar_paquete(paquete);
    }
}

static void dormir(int duracion){
    reloj = temporal_create();

    while(temporal_gettime(reloj) <= duracion){
        if(proceso_eliminado){
                log_info(generica_logger, "El proceso fue finalizado\n");
                temporal_destroy(reloj);
                return;
        }

    }

    int termino_io = 1;
    send(socket_kernel, &termino_io, sizeof(int), 0);
    log_info(generica_logger, "El proceso finalizo IO\n");

    temporal_destroy(reloj);
}