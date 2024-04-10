#include "io.h"

// Variables Locales //
static char* ip_kernel;
static char* puerto_kernel;
static char* ip_memoria;
static char* puerto_memoria;
static int socket_kernel;
static int socket_memoria;
static int proceso_conectado;
static uint32_t direccion_fisica;

// Funciones Locales //
static void realizar_lectura();
static void pedir_lectura (uint32_t direccion_fisica);

// Main //
void iniciar_interfaz_stdin(char* nombre, t_config* config) {
    
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    
    log_info(io_logger, "Iniciando interfaz STDIN: %s", nombre);
    
    // COMUNICACION KERNEL //
    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    // COMUNICACION MEMORIA //
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);

    realizar_lectura();
}

// Tenemos que escribir en alguna parte un texto, en la consola?
static void realizar_lectura() 
{
    t_paquete* paquete = recibir_paquete(socket_kernel);
    void* stream = paquete->buffer->stream;

    if(paquete->codigo_operacion == STDIN_READ)
    {
        proceso_conectado = sacar_entero_de_paquete(&stream);
        direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
        
        eliminar_paquete(paquete);
        pedir_lectura(direccion_fisica);
    } 
    else { 
        eliminar_paquete(paquete);
        log_error(io_logger, "El paquete no es de tipo STDIN_READ");
    }
}

static void pedir_lectura(uint32_t direccion_fisica) 
{
    t_paquete* paquete = crear_paquete(HACER_LECTURA);
    //agregar_cadena_a_paquete(paquete, texto);
    agregar_entero_a_paquete(paquete,direccion_fisica);
    enviar_paquete(paquete, socket_memoria);
}