#include "memoria.h"

void* espacio_usuario;
t_list* procesos_en_memoria;
t_list* marcos;

static void crear_marcos_memoria();

//============================================ ESPACIO DE USUARIO ===========================================================
/*void creacion_espacio_usuario()
{
	espacio_usuario = malloc (config_valores_memoria.tam_memoria); //PUEDE SER CALLOC
	if (espacio_usuario == NULL) {
        perror ("No se pudo alocar memoria al espacio de usuario.");
        abort();
    } 
}*/

void creacion_espacio_usuario(){
	espacio_usuario = malloc(config_valores_memoria.tam_memoria);

	memset(espacio_usuario,0,config_valores_memoria.tam_memoria); //inicializa con 0

	marcos = list_create();

	crear_marcos_memoria();
}

//============================================ TABLAS DE PAGINAS & PROCESOS ====================================================
void crear_estructuras_memoria(int pid, FILE* archivo)
{
	t_proceso_en_memoria* proceso = malloc(sizeof(t_proceso_en_memoria));
	proceso->pid = pid;
	proceso->instrucciones = list_create();
    proceso->paginas_en_memoria = list_create();

	leer_instrucciones_desde_archivo(proceso, archivo);

	list_add(procesos_en_memoria, proceso);
}

static void crear_marcos_memoria() 
{
	for(int i = 0; i < cantidad_marcos; i++) {
		t_marco* marco = malloc(sizeof(* marco)); //No deberia ser sizeof t_marco

		marco->nro_pag = -1;
		marco->nro_marco = i;
		marco->libre = 1;
		marco->pid_proceso = -1;
        marco->cantidad_bytes_libres = config_valores_memoria.tam_pagina;
		list_add(marcos, marco);
	}
}

