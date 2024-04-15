
#include "io.h"

static char *ip_kernel;
static char *puerto_kernel;
static int socket_kernel;
static int tiempo_unidad_de_trabajo;

static void realizar_sleep();

void main_generica(t_interfaz* interfaz){
    char* nombre_interfaz = interfaz->nombre_interfaz;
    t_config* config_interfaz = interfaz->config_interfaz;
    

    ip_kernel = config_get_string_value(config_interfaz, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config_interfaz, "PUERTO_KERNEL");
    tiempo_unidad_de_trabajo = config_get_int_value(config_interfaz, "TIEMPO_UNIDAD_TRABAJO");

    log_info(io_logger, "Iniciando interfaz generica: %s\n", nombre_interfaz);

    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);

    conectarse_a_kernel(socket_kernel, INTERFAZ_GENERICA ,nombre_interfaz, "GENERICA");
    
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

        eliminar_paquete(paquete);

        log_info(io_logger, "PID: %d - Operacion: IO_GEN_SLEEP\n", proceso_conectado);

        usleep(tiempo_unidad_de_trabajo * cantidad_tiempo * 1000);

        printf("Estuvo buena la siesta\n");

        send(socket_kernel, &proceso_conectado, sizeof(int), 0);
        } else {
            eliminar_paquete(paquete);
        }
    }
}
