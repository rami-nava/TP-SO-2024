#include "kernel.h"

//================================================= Variables Globales =====================================================================
t_log* kernel_logger;
t_config* config;
int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_memoria;
int socket_filesystem;
arch_config_kernel config_valores_kernel;
pthread_t consola;
pthread_t largo_plazo;
pthread_t corto_plazo;

//========================================================================================================================================
int main(void)
{
	kernel_logger = log_create("/home/utnso/tp-2024-1c-SegmenFault/kernel/cfg/kernel.log", "kernel.log", 1, LOG_LEVEL_INFO);

	cargar_configuracion("/home/utnso/tp-2024-1c-SegmenFault/kernel/cfg/kernel.config");

    //CPU
    socket_cpu_dispatch = crear_conexion(config_valores_kernel.ip_cpu, config_valores_kernel.puerto_cpu_dispatch);
	socket_cpu_interrupt = crear_conexion(config_valores_kernel.ip_cpu, config_valores_kernel.puerto_cpu_interrupt);

    //MEMORIA
    socket_memoria = crear_conexion(config_valores_kernel.ip_memoria, config_valores_kernel.puerto_memoria);

    //FILESYSTEM
    //socket_filesystem = crear_conexion(config_valores_kernel.ip_filesystem, config_valores_kernel.puerto_filesystem);

    inicializar_planificador();
    
    pthread_create(&largo_plazo, NULL, (void* ) planificador_largo_plazo, NULL);
    pthread_create(&corto_plazo, NULL, (void* ) planificador_corto_plazo_segun_algoritmo, NULL);
    pthread_create(&consola, NULL, (void* ) inicializar_consola_interactiva, NULL);
   
   using_history(); // Inicializar la historia de comando

   pthread_join(largo_plazo,NULL);
   pthread_join(corto_plazo,NULL);
   pthread_join(consola, NULL);

   return EXIT_SUCCESS;
}

