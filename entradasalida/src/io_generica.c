
#include "io.h"

static int socket_kernel;
static int tiempo_unidad_de_trabajo;
static char* ip_kernel;
static char* puerto_kernel;
static int proceso_conectado;
static int cantidad_tiempo;

static void realizar_sleep();

void iniciar_interfaz_generica(char* nombre, t_config* config) {
    
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    tiempo_unidad_de_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");

    log_info(io_logger, "Iniciando interfaz generica: %s", nombre);

    // COMUNICACION KERNEL //
    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    
    realizar_sleep();
}

void realizar_sleep() 
{
    t_paquete* paquete = recibir_paquete(socket_kernel);
    void* stream = paquete->buffer->stream;

    if(paquete->codigo_operacion == GENERICA_IO_SLEEP) {

    proceso_conectado = sacar_entero_de_paquete(&stream);
    cantidad_tiempo = sacar_entero_de_paquete(&stream);

    eliminar_paquete(paquete);

    log_info(io_logger, "PID: %d - Operacion: IO_GEN_SLEEP", proceso_conectado);

    usleep(tiempo_unidad_de_trabajo * cantidad_tiempo * 1000);
    } else {
        eliminar_paquete(paquete);
        log_error(io_logger, "Comando invalido. Debe ser GENERICA_IO_SLEEP");
    }
}
