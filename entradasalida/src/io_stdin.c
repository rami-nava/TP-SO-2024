#include "io.h"

// Funciones Locales //
static void realizar_escritura();
static void guardar_escritura(uint32_t direccion_fisica, uint32_t tamanio_registro);

static char *ip_kernel;
static char *puerto_kernel;
static char *ip_memoria;
static char *puerto_memoria;
static int socket_kernel;
static int socket_memoria;

void main_stdin(t_interfaz* interfaz_hilo) 
{
    char* nombre = interfaz_hilo->nombre_interfaz;
    t_config* config = interfaz_hilo->config_interfaz;
    
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    
    log_info(io_logger, "Iniciando interfaz STDIN: %s", nombre);
    
    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    //socket_memoria = crear_conexion(ip_memoria, puerto_memoria);

    conectarse_a_kernel(socket_kernel, INTERFAZ_STDIN,nombre, "STDIN");

    realizar_escritura();
}

static void realizar_escritura() 
{
    while (1)
    {
        t_paquete* paquete = recibir_paquete(socket_kernel);
        void* stream = paquete->buffer->stream;

        if(paquete->codigo_operacion == STDIN_READ)
        {
            int proceso_conectado = sacar_entero_de_paquete(&stream);
            uint32_t direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
            uint32_t tamanio_registro = sacar_entero_sin_signo_de_paquete(&stream);
            
            eliminar_paquete(paquete);

            log_info(io_logger, "PID: %d - Operacion: IO_STDIN_READ\n", proceso_conectado);

            guardar_escritura(direccion_fisica, tamanio_registro, socket_memoria);
        } 
        else { 
            eliminar_paquete(paquete);
            log_error(io_logger, "El paquete no es de tipo STDIN_READ");
        }

    }
    
}

static void guardar_escritura(uint32_t direccion_fisica, uint32_t tamanio_registro) 
{
    char* leer_linea;
    char texto_a_guardar[tamanio_registro];

    leer_linea = readline("Ingrese el texto que desea guardar en memoria > \n");

    if(leer_linea){
        strncpy(texto_a_guardar, leer_linea, tamanio_registro); //Solo se va a guardar dependiendo de la cantidad especificada en el parametro
        texto_a_guardar[tamanio_registro] = '\0';
        free(leer_linea);
    }

    t_paquete* paquete = crear_paquete(HACER_LECTURA);
    agregar_cadena_a_paquete(paquete, texto_a_guardar);
    agregar_entero_a_paquete(paquete,direccion_fisica);
    enviar_paquete(paquete, socket_memoria);
    
    int escritura_guardada = 0;
    recv(socket_memoria, &escritura_guardada, sizeof(int), 0); //Memoria confirma que guardo el texto en la direccion especificada

    send(socket_kernel, &escritura_guardada, sizeof(int), 0); //Le avisa a kernel que el texto fue guardado en memoria
}