#include "io.h"

// Variables Locales //
static char *ip_kernel;
static char *puerto_kernel;
static char *ip_memoria;
static char *puerto_memoria;
static int socket_kernel;
static int socket_memoria;
static int tiempo_unidad_de_trabajo;
t_log* stdout_logger;

// Funciones Locales //
static void solicitar_informacion_memoria();
static void pedir_lectura(uint32_t direccion_fisica, uint32_t tamanio); 
static void* recibir_lectura(uint32_t tamanio); 

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
    tiempo_unidad_de_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    
    log_info(stdout_logger, "Iniciando interfaz STDOUT: %s", nombre);
        
    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);

    conectarse_a_kernel(socket_kernel, INTERFAZ_STDOUT,nombre, "STDOUT");

    // Realiza su IO_STDOUT_WRITE
    solicitar_informacion_memoria();

}   

static void solicitar_informacion_memoria ()
{
    while (1)
    {
        t_paquete* paquete = recibir_paquete(socket_kernel);
        void* stream = paquete->buffer->stream;

        if(paquete->codigo_operacion == STDOUT_WRITE)
        {
            int proceso_conectado = sacar_entero_de_paquete(&stream);
            uint32_t direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
            uint32_t tamanio = sacar_entero_sin_signo_de_paquete(&stream);
            
            eliminar_paquete(paquete);

            log_info(stdout_logger, "PID: %d - Operacion: IO_STDOUT_WRITE\n", proceso_conectado);

            //Tarda una unidad de trabajo
            usleep(tiempo_unidad_de_trabajo * 1000);

            //Le pide la lectura de esa direccion a la memoria 
            pedir_lectura(direccion_fisica, tamanio);

            //Recibe la lectura de la memoria
            char* lectura = malloc(sizeof(tamanio));

            lectura = recibir_lectura(tamanio);

            //Mostramos por pantalla la lectura
            printf("Lectura realizada: %s\n", lectura);

            free(lectura);
            
            //Le avisa a Kernel que ya se realizo la lectura, y ya se mostro por pantalla
            send(socket_kernel, &proceso_conectado, sizeof(int), 0); 
        } 
        else { 
            eliminar_paquete(paquete);
            log_error(stdout_logger, "El paquete no es de tipo STDOUT_WRITE");
        }
    }
    
}

static void pedir_lectura(uint32_t direccion_fisica, uint32_t tamanio) 
{
    t_paquete* paquete = crear_paquete(REALIZAR_LECTURA);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    enviar_paquete(paquete, socket_memoria);
}

static void* recibir_lectura(uint32_t tamanio) 
{
    while (1)
    {
        t_paquete* paquete = recibir_paquete(socket_memoria);
        void* stream = paquete->buffer->stream;

        if(paquete->codigo_operacion == DEVOLVER_LECTURA){
            void* texto_leido = sacar_bytes_de_paquete(&stream, tamanio);

            return texto_leido;
        } else
        {
            log_error(stdout_logger, "El paquete no es del tipo DEVOLVER_LECTURA");
            return NULL;
        }

        eliminar_paquete(paquete);
    }
}
