#include "memoria.h"

void* espacio_usuario;

//============================================ ESPACIO DE USUARIO ===========================================================
void creacion_espacio_usuario()
{
	espacio_usuario = malloc (config_valores_memoria.tam_memoria); //PUEDE SER CALLOC
	if (espacio_usuario == NULL) {
        perror ("No se pudo alocar memoria al espacio de usuario.");
        abort();
    } 
}

//============================================ TABLAS DE PAGINAS & PROCESOS ====================================================
void crear_estructuras_memoria(int pid, FILE* archivo)
{
	t_proceso_en_memoria* proceso = malloc(sizeof(t_proceso_en_memoria));
	proceso->pid = pid;
	proceso->instrucciones = list_create();

	leer_instrucciones_desde_archivo(proceso, archivo);

	//crear_tablas_paginas_proceso(proceso);

	list_add(procesos_en_memoria, proceso);
}

