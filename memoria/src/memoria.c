#include "memoria.h"

//VARIABLES GLOBALES
t_log* memoria_logger;
t_config* config;
int server_memoria;
arch_config config_valores_memoria;
int cantidad_marcos;

int main(void) {
	
    memoria_logger = log_create("/home/utnso/tp-2024-1c-SegmenFault/memoria/cfg/memoria.log", "memoria.log", 1, LOG_LEVEL_INFO);

    cargar_configuracion("/home/utnso/tp-2024-1c-SegmenFault/memoria/cfg/memoria.config");

    creacion_espacio_usuario();

    procesos_en_memoria = list_create();

	cantidad_marcos = config_valores_memoria.tam_memoria / config_valores_memoria.tam_pagina;

    server_memoria = iniciar_servidor(config_valores_memoria.ip_memoria,config_valores_memoria.puerto_escucha);

    while(1) 
    {
        atender_clientes_memoria(server_memoria);
    }

	return 0;
}