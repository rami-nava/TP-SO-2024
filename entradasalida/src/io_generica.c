
#include "io.h"

static void realizar_sleep(int socket, int tiempo_trabajo);
static void conectarse_a_kernel(int socket, char* nombre);
static void* conexion_hilo_kernel(t_interfaz* interfaz_hilo);

void iniciar_interfaz_generica(t_interfaz* interfaz) {
    pthread_t hilo_interfaz;
    pthread_create(&hilo_interfaz, NULL, conexion_hilo_kernel, interfaz);
    pthread_detach(hilo_interfaz);
}

void* conexion_hilo_kernel(t_interfaz* interfaz_hilo){
    char* nombre_hilo_interfaz = interfaz_hilo->nombre_interfaz;
    t_config* config_hilo_interfaz = interfaz_hilo->config_interfaz;
    int socket_kernel;
    int tiempo_unidad_de_trabajo;
    char* ip_kernel;
    char* puerto_kernel;

    ip_kernel = config_get_string_value(config_hilo_interfaz, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config_hilo_interfaz, "PUERTO_KERNEL");
    tiempo_unidad_de_trabajo = config_get_int_value(config_hilo_interfaz, "TIEMPO_UNIDAD_TRABAJO");

    log_info(io_logger, "Iniciando interfaz generica: %s\n", nombre_hilo_interfaz);

    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);

    conectarse_a_kernel(socket_kernel, nombre_hilo_interfaz);
    
    realizar_sleep(socket_kernel, tiempo_unidad_de_trabajo);
}

void realizar_sleep(int socket_kernel,int tiempo_unidad_de_trabajo) 
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

        eliminar_paquete(paquete);

        log_info(io_logger, "PID: %d - Operacion: IO_GEN_SLEEP\n", proceso_conectado);

        usleep(tiempo_unidad_de_trabajo * cantidad_tiempo * 1000);

        log_info(io_logger, "Estuvo buena la siesta\n");
        int termino_de_mimir = 1;
        send(socket_kernel, &termino_de_mimir, sizeof(int), 0);
        } else {
            eliminar_paquete(paquete);
        }
    }
}

void conectarse_a_kernel(int socket_kernel, char* nombre)
{
    t_paquete* paquete = crear_paquete(INTERFAZ_GENERICA);
    agregar_cadena_a_paquete(paquete, nombre);
    agregar_cadena_a_paquete(paquete, "GENERICA");
    enviar_paquete(paquete, socket_kernel);
}
