#include "kernel.h"

//================================================= Variables Globales =====================================================================
t_log* kernel_logger;
t_config* config;
int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_memoria;
int servidor_kernel;

arch_config_kernel config_valores_kernel;
pthread_t consola;
pthread_t largo_plazo;
pthread_t corto_plazo;
pthread_t hilo_servidor_kernel_io;

//========================================================================================================================================
int main(void)
{
	kernel_logger = log_create("/home/utnso/tp-2024-1c-SegmenFault/kernel/cfg/kernel.log", "kernel.log", 1, LOG_LEVEL_INFO);

	cargar_configuracion("/home/utnso/tp-2024-1c-SegmenFault/kernel/cfg/kernel.config");

     //PLANIFICADORES
    inicializar_planificador();

    //LISTAS
    inicializar_listas();

    //CPU
    socket_cpu_dispatch = crear_conexion(config_valores_kernel.ip_cpu, config_valores_kernel.puerto_cpu_dispatch);
    socket_cpu_interrupt = crear_conexion(config_valores_kernel.ip_cpu, config_valores_kernel.puerto_cpu_interrupt);

    //MEMORIA
    socket_memoria = crear_conexion(config_valores_kernel.ip_memoria, config_valores_kernel.puerto_memoria);

    //MULTIHILOS
    pthread_create(&largo_plazo, NULL, (void* ) planificador_largo_plazo, NULL);
    pthread_create(&corto_plazo, NULL, (void* ) planificador_corto_plazo_segun_algoritmo, NULL);
    pthread_create(&consola, NULL, (void* ) inicializar_consola_interactiva, NULL);
    pthread_create(&hilo_servidor_kernel_io, NULL, (void* ) servidor_kernel_io, NULL);
   
    //using_history(); // Inicializar la historia de comando

    pthread_detach(largo_plazo);
    pthread_detach(corto_plazo);
    pthread_detach(hilo_servidor_kernel_io);
    if(!strcmp(config_valores_kernel.algoritmo, "RR") || !strcmp(config_valores_kernel.algoritmo, "VRR"))
         inicializar_reloj_RR();
    pthread_join(consola, NULL);

    return EXIT_SUCCESS;
}