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
	config_valores_memoria.retardo_respuesta=config_get_int_value(config,"RETARDO_RESPUESTA");
}

// ======================================= PROCESOS =====================================================================
t_proceso_en_memoria *buscar_proceso(t_list *lista, int pid_buscado)
{
	int elementos = list_size(lista);
	for (int i = 0; i < elementos; i++)
	{
		t_proceso_en_memoria *proceso = list_get(lista, i);
		if (pid_buscado == proceso->pid)
		{
			return proceso;
		}
	}
	log_error(memoria_logger, "Proceso no encontrado\n");
	return NULL;
}

void finalizar_en_memoria(int pid){

	//Obtengo el proceso por el pid
	t_proceso_en_memoria *proceso = buscar_proceso(procesos_en_memoria, pid);

	//limpiar_tabla_paginas(proceso);

	//Elimino el proceso de la lista
	list_remove_element(procesos_en_memoria, proceso);

	//Limpio su lista de instrucciones
	list_destroy(proceso->instrucciones);

	//Libero su memoria
	free(proceso);
}