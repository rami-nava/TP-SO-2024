#include "memoria.h"

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
	config_valores_memoria.retardo_respuesta=config_get_string_value(config,"RETARDO_RESPUESTA");
}

//======================================= SEMÁFOROS ============================================================

void inicializar_semaforos()
{
    pthread_mutex_init(&mutex_instrucciones, NULL);
    pthread_mutex_init(&mutex_lista_instrucciones, NULL);
    pthread_mutex_init(&mutex_path, NULL);
	//pthread_mutex_init(&mutex_tiempo, NULL);

	//sem_init(&(swap_creado), 0, 0);
    sem_init(&(solucionado_pf), 0, 0);
	//sem_init(&(swap_finalizado), 0, 0);
}