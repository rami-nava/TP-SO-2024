#include "io.h"

// Funciones Locales //
static void realizar_lectura(int socket_kernel, int socket_memoria);
static void pedir_lectura (uint32_t direccion_fisica, int socket_memoria);

void main_stdin(t_interfaz* interfaz_hilo) 
{
    char* nombre = interfaz_hilo->nombre_interfaz;
    t_config* config = interfaz_hilo->config_interfaz;
    char* ip_kernel;
    char* puerto_kernel;
    char* ip_memoria;
    char* puerto_memoria;
    int socket_kernel;
    int socket_memoria;
    
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    
    log_info(io_logger, "Iniciando interfaz STDIN: %s", nombre);
    
    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);

    conectarse_a_kernel(socket_kernel, nombre, "STDIN");

    realizar_lectura(socket_kernel, socket_memoria);
}

// Tenemos que escribir en alguna parte un texto, en la consola?
static void realizar_lectura(int socket_kernel, int socket_memoria) 
{
    while (1)
    {
        t_paquete* paquete = recibir_paquete(socket_kernel);
        void* stream = paquete->buffer->stream;

        if(paquete->codigo_operacion == STDIN_READ)
        {
            int proceso_conectado = sacar_entero_de_paquete(&stream);
            uint32_t direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
            
            eliminar_paquete(paquete);

            log_info(io_logger, "PID: %d - Operacion: IO_STDIN_READ\n", proceso_conectado);

            pedir_lectura(direccion_fisica, socket_memoria);
        } 
        else { 
            eliminar_paquete(paquete);
            log_error(io_logger, "El paquete no es de tipo STDIN_READ");
        }

    }
    
}

static void pedir_lectura(uint32_t direccion_fisica, int socket_memoria) 
{
    t_paquete* paquete = crear_paquete(HACER_LECTURA);
    //agregar_cadena_a_paquete(paquete, texto);
    agregar_entero_a_paquete(paquete,direccion_fisica);
    enviar_paquete(paquete, socket_memoria);
}