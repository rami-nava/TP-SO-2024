#include "io.h"

// Funciones Locales //
static void solicitar_informacion_memoria();
static char* pedir_lectura(uint32_t direccion_fisica, uint32_t tamanio); 

static char *ip_kernel;
static char *puerto_kernel;
static char *ip_memoria;
static char *puerto_memoria;
static int socket_kernel;
static int socket_memoria;
static int tiempo_unidad_de_trabajo;

// Funciones Globales //
void main_stdout(t_interfaz* interfaz_hilo) 
{
    char* nombre = interfaz_hilo->nombre_interfaz;
    t_config* config = interfaz_hilo->config_interfaz;
    
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    tiempo_unidad_de_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    
    log_info(io_logger, "Iniciando interfaz STDOUT: %s", nombre);
        
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

            log_info(io_logger, "PID: %d - Operacion: IO_STDOUT_WRITE\n", proceso_conectado);

            //Tarda una unidad de trabajo
            usleep(tiempo_unidad_de_trabajo * 1000);

            //Por donde muestro el resultado
            char* lectura = pedir_lectura(direccion_fisica, tamanio);

            //Mostrar el resultado?
            printf("Lectura realizada: %s\n", lectura);

            int lectura_realizada = 1;
            send(socket_kernel, &lectura_realizada, sizeof(int), 0); //Le avisa a Kernel que ya se realizo la lectura, y ya se mostro por pantalla
        } 
        else { 
            eliminar_paquete(paquete);
            log_error(io_logger, "El paquete no es de tipo STDOUT_WRITE");
        }
    }
    
}

static char* pedir_lectura(uint32_t direccion_fisica, uint32_t tamanio) 
{
    t_paquete* paquete = crear_paquete(HACER_LECTURA);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);

    paquete = recibir_paquete(socket_kernel);
    if(paquete->codigo_operacion == DEVOLVER_LECTURA){
        void* stream = paquete->buffer->stream;
        char* texto_leido = sacar_cadena_de_paquete(&stream);
        eliminar_paquete(paquete);

        return texto_leido;
    }
    else{
        eliminar_paquete(paquete);
        log_error(io_logger, "El paquete no es del tipo DEVOLVER_LECTURA");

        return NULL;
    }
}