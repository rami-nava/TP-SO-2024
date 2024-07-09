#include "memoria.h"

//VARIABLES GLOBALES
t_log* memoria_logger;
t_config* config;
int server_memoria;
arch_config config_valores_memoria;
int cantidad_marcos;
uint32_t tam_pagina;
t_list* procesos_en_memoria;

int main(void) {
	
    memoria_logger = log_create("/home/utnso/tp-2024-1c-SegmenFault/memoria/cfg/memoria.log", "memoria.log", 1, LOG_LEVEL_INFO);

    cargar_configuracion("/home/utnso/tp-2024-1c-SegmenFault/memoria/cfg/memoria.config");

    procesos_en_memoria = list_create();

	pthread_mutex_init(&mutex_PROCESOS_EN_MEMORIA, NULL);
	pthread_mutex_init(&mutex_espacio_usuario, NULL);

    tam_pagina = config_valores_memoria.tam_pagina;

	cantidad_marcos = config_valores_memoria.tam_memoria / tam_pagina;

    creacion_espacio_usuario();

    server_memoria = iniciar_servidor(config_valores_memoria.ip_memoria,config_valores_memoria.puerto_escucha);

    while(1) 
    {
        atender_clientes_memoria(server_memoria);
    }

	return 0;
}


//======================================= CONFIG ============================================================
void cargar_configuracion(char* path){
	
	config = config_create(path); 

	      if (config == NULL) {
	          perror("Archivo de configuracion de filesystem no encontrado \n");
	          abort();
	      }
	config_valores_memoria.ip_memoria=config_get_string_value(config,"IP_MEMORIA");
	config_valores_memoria.puerto_escucha=config_get_string_value(config,"PUERTO_ESCUCHA");
	config_valores_memoria.tam_memoria=config_get_int_value(config,"TAM_MEMORIA");
	config_valores_memoria.tam_pagina=config_get_int_value(config,"TAM_PAGINA");
	config_valores_memoria.path_instrucciones=config_get_string_value(config,"PATH_INSTRUCCIONES");
	config_valores_memoria.retardo_respuesta=config_get_int_value(config,"RETARDO_RESPUESTA");
}