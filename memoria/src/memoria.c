#include "memoria.h"

//VARIABLES GLOBALES
t_log* memoria_logger;
t_config* config;
int socket_memoria;
int server_fd;
arch_config config_valores_memoria;
sem_t solucionado_pf;
t_list* procesos_en_memoria;

int main(void) {
	
    memoria_logger = log_create("/home/utnso/tp-2024-1c-SegmenFault/memoria/cfg/memoria.log", "memoria.log", 1, LOG_LEVEL_INFO);

    ///CARGAR LA CONFIGURACION
    cargar_configuracion("/home/utnso/tp-2024-1c-SegmenFault/memoria/cfg/memoria.config");

    //creacion_espacio_usuario();

    procesos_en_memoria = list_create();

    inicializar_semaforos();

    int server_memoria = iniciar_servidor(config_valores_memoria.ip_memoria,config_valores_memoria.puerto_escucha);

    while(1) 
    {
        atender_clientes_memoria(server_memoria);
    }

	return 0;
}


