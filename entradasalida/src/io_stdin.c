#include "io.h"

// Funciones Locales //
static void realizar_escritura();
static void guardar_escritura();

static char *ip_kernel;
static char *puerto_kernel;
static char *ip_memoria;
static char *puerto_memoria;
static int socket_kernel;
static int socket_memoria;
t_log* stdin_logger;
static uint32_t direccion_fisica;
static uint32_t tamanio_registro;

void main_stdin(t_interfaz* interfaz_hilo) 
{    
    char* nombre = interfaz_hilo->nombre_interfaz;
    t_config* config = interfaz_hilo->config_interfaz;

    char path[70] = "/home/utnso/tp-2024-1c-SegmenFault/entradasalida/cfg/";

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
            direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
            tamanio_registro = sacar_entero_sin_signo_de_paquete(&stream);

            log_info(stdin_logger, "PID: %d - Operacion: IO_STDIN_READ\n", proceso_conectado);
            
            guardar_escritura();
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
    char* leer_linea;
    char texto_a_guardar[tamanio_registro];

    leer_linea = readline("Ingrese el texto que desea guardar en memoria > \n");
    
    if(leer_linea){
        strncpy(texto_a_guardar, leer_linea, tamanio_registro); //Solo se va a guardar dependiendo de la cantidad especificada en el parametro
        texto_a_guardar[tamanio_registro] = '\0';
        free(leer_linea);

        log_info(stdin_logger, "Guardando texto en memoria: %s\n", texto_a_guardar);
        
        //IO solicita que memoria guarde el texto en la direccion especificada
        t_paquete* paquete = crear_paquete(REALIZAR_ESCRITURA);
        agregar_entero_sin_signo_a_paquete(paquete,direccion_fisica);
        agregar_entero_a_paquete(paquete, tamanio_registro);
        agregar_bytes_a_paquete(paquete, texto_a_guardar, tamanio_registro);
        enviar_paquete(paquete, socket_memoria);
        
        //Memoria confirma que guardo el texto en la direccion especificada
        int escritura_guardada;
        recv(socket_memoria, &escritura_guardada, sizeof(int), 0); 

        //Le avisa a kernel que el texto fue guardado en memoria
        int termino_io = 1;
        send(socket_kernel, &termino_io, sizeof(int), 0); 
    }
}


void desconectar_memoria_stdin(){
    t_paquete* paquete = crear_paquete(DESCONECTAR_IO);
    agregar_entero_a_paquete(paquete,1);
    enviar_paquete(paquete, socket_memoria);
}