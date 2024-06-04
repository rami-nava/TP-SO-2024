#include "io.h"

// Variables Locales //
static char *ip_kernel;
static char *puerto_kernel;
static char *ip_memoria;
static char *puerto_memoria;
static int socket_kernel;
static int socket_memoria;
t_log* stdout_logger;
static uint32_t direccion_fisica;
static uint32_t tamanio_registro;

// Funciones Locales //
static void stdout_write();
static void pedir_lectura(uint32_t direccion_fisica, uint32_t tamanio, int pid); 
static char* recibir_lectura(uint32_t tamanio);
static void leer_memoria(int pid); 

void main_stdout(t_interfaz* interfaz_hilo) 
{
    char* nombre = interfaz_hilo->nombre_interfaz;
    t_config* config = interfaz_hilo->config_interfaz;

    char path[70] = "/home/utnso/tp-2024-1c-SegmenFault/entradasalida/cfg/";

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

    // Realiza su IO_STDOUT_WRITE
    stdout_write();

}   

static void stdout_write ()
{
    while (1)
    {
        t_paquete* paquete = recibir_paquete(socket_kernel);
        void* stream = paquete->buffer->stream;

        if(paquete->codigo_operacion == STDOUT_WRITE)
        {
            int pid = sacar_entero_de_paquete(&stream);
            direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
            tamanio_registro = sacar_entero_sin_signo_de_paquete(&stream);

            log_info(stdout_logger, "PID: %d - Operacion: IO_STDOUT_WRITE\n", pid);

            leer_memoria(pid);
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

static void leer_memoria(int pid){
   
    //Le pide la lectura de esa direccion a la memoria 
    pedir_lectura(direccion_fisica, tamanio_registro, pid);

    //Recibe la lectura de la memoria
    char* lectura = recibir_lectura(tamanio_registro);

    //Mostramos por pantalla la lectura
    printf("Lectura realizada: %s\n", lectura);

    free(lectura);
            
    //Le avisa a Kernel que ya se realizo la lectura, y ya se mostro por pantalla
    int termino_io = 1;
    send(socket_kernel, &termino_io, sizeof(int), 0); 
}

static void pedir_lectura(uint32_t direccion_fisica, uint32_t tamanio, int pid) 
{
    t_paquete* paquete = crear_paquete(REALIZAR_LECTURA);
    agregar_entero_a_paquete(paquete, pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    enviar_paquete(paquete, socket_memoria);
}

static char* recibir_lectura(uint32_t tamanio) 
{
    t_paquete* paquete = recibir_paquete(socket_memoria);
    void* stream = paquete->buffer->stream;

    if(paquete->codigo_operacion == RESULTADO_LECTURA){
        char* lectura = sacar_cadena_de_paquete(&stream);

        eliminar_paquete(paquete);
        return lectura;
    }
    else {
        log_error(stdout_logger, "No me enviaste el contenido \n");
        eliminar_paquete(paquete);
        abort();
    }
}

/*
static char* recibir_lectura(uint32_t tamanio) 
{
    int cod_op = recibir_operacion(socket_memoria);

    if (cod_op == RESULTADO_LECTURA){

        void* buffer = recibir_buffer(socket_memoria);

        char* lectura = malloc(tamanio);
        memcpy(lectura, buffer, tamanio);

        free(buffer);

        return lectura;
    }
    else {
        log_error(stdout_logger, "No me enviaste el contenido \n");
        abort();
    }
}*/

void desconectar_memoria_stdout(){
    t_paquete* paquete = crear_paquete(DESCONECTAR_IO);
    agregar_entero_a_paquete(paquete,1);
    enviar_paquete(paquete, socket_memoria);
}