#include "memoria.h"

//VARIABLES GLOBALES
t_log* memoria_logger;
t_config* config;
int server_memoria;
arch_config config_valores_memoria;

int main(void) {
	
    memoria_logger = log_create("/home/utnso/tp-2024-1c-SegmenFault/memoria/cfg/memoria.log", "memoria.log", 1, LOG_LEVEL_INFO);

    cargar_configuracion("/home/utnso/tp-2024-1c-SegmenFault/memoria/cfg/memoria.config");

    creacion_espacio_usuario();

    procesos_en_memoria = list_create();

    server_memoria = iniciar_servidor(config_valores_memoria.ip_memoria,config_valores_memoria.puerto_escucha);

    while(1) 
    {
        atender_clientes_memoria(server_memoria);
    }

	return 0;
}

void creacion_espacio_usuario(){
	espacio_usuario = malloc(config_valores_memoria.tam_memoria);

	memset(espacio_usuario,0,config_valores_memoria.tam_memoria); //inicializa con 0

	marcos = list_create();

	crear_marcos_memoria(cant_marcos());
}

int cant_marcos(){
	return config_valores_memoria.tam_memoria / config_valores_memoria.tam_pagina;
}

void crear_marcos_memoria(int cantidad_marcos) {
	for(int i = 0; i < cantidad_marcos; i++) {
		t_marco* marco = malloc(sizeof(* marco));

		marco->nro_pag = -1;
		marco->nro_marco = i;
		marco->libre = 1;
		marco->pid_proceso = -1;
        marco->cantidad_bytes_libres = config_valores_memoria.tam_pagina;
		list_add(marcos, marco);
	}
}

void crear_estructuras_memoria(int pid, FILE* archivo)
{
	t_proceso_en_memoria* proceso = malloc(sizeof(t_proceso_en_memoria));
	proceso->pid = pid;
	proceso->instrucciones = list_create();
    proceso->paginas_en_memoria = list_create();

	leer_instrucciones_desde_archivo(proceso, archivo);

	list_add(procesos_en_memoria, proceso);
}
