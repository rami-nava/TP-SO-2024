#include "cpu.h"

pthread_t hilo_interrupt;
pthread_t cpu;
int socket_cliente_memoria;
int socket_servidor_dispatch;
int socket_servidor_interrupt;
int socket_cliente_dispatch;
int socket_cliente_interrupt;
t_log *cpu_logger;

t_list* tlb;
int cantidad_entradas_tlb;
char* path_config;

int main(void)
{
    cpu_logger = log_create("/home/utnso/tp-2024-1c-SegmenFault/cpu/cfg/cpu.log", "cpu.log", 1, LOG_LEVEL_INFO);

    path_config = "/home/utnso/tp-2024-1c-SegmenFault/cpu/cfg/cpu.config";
    
    cargar_configuracion(path_config);

    inicializar_semaforos();

    socket_cliente_memoria = crear_conexion(config_valores_cpu.ip_memoria, config_valores_cpu.puerto_memoria);

    realizar_handshake();

    cantidad_entradas_tlb =  config_valores_cpu.cantidad_entradas_tlb;
    tlb = list_create();

    socket_servidor_dispatch = iniciar_servidor(config_valores_cpu.ip_cpu, config_valores_cpu.puerto_escucha_dispatch);
    socket_servidor_interrupt = iniciar_servidor(config_valores_cpu.ip_cpu, config_valores_cpu.puerto_escucha_interrupt);

    socket_cliente_dispatch = esperar_cliente(socket_servidor_dispatch);
    socket_cliente_interrupt = esperar_cliente(socket_servidor_interrupt);

    pthread_create(&hilo_interrupt, NULL, (void *)atender_interrupt, &socket_cliente_interrupt);
    pthread_detach(hilo_interrupt);

    pthread_create(&cpu, NULL, (void* ) atender_dispatch, NULL);
    pthread_join(cpu, NULL);

    return 0;

}
