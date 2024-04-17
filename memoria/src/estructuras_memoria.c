#include "memoria.h"

void* espacio_usuario;
t_list* procesos_en_memoria;
t_list* marcos;



//============================================ ESPACIO DE USUARIO ===========================================================


void creacion_espacio_usuario(){
	espacio_usuario = malloc(config_valores_memoria.tam_memoria);

	if (espacio_usuario == NULL) {
        perror ("No se pudo alocar memoria al espacio de usuario.");
        abort();
    } 

	memset(espacio_usuario,0,config_valores_memoria.tam_memoria); //inicializa con 0

	marcos = list_create();

	crear_marcos_memoria();
}

//============================================ PROCESOS EN MEMORIA - MARCOS ====================================================

void crear_estructuras_memoria(int pid, FILE* archivo){
	t_proceso_en_memoria* proceso = malloc(sizeof(t_proceso_en_memoria));
	proceso->pid = pid;
	proceso->instrucciones = list_create();
    proceso->paginas_en_memoria = list_create();

	leer_instrucciones_desde_archivo(proceso, archivo);

	list_add(procesos_en_memoria, proceso);
}

void crear_marcos_memoria() {
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


//============================================ FUNCIONES DE LOGICA DE MARCOS/PAGINAS =============================================


int cantidad_de_marcos_libres(){
	
	t_marco* marco_obtenido = malloc(sizeof(t_marco));
	int contador=0;
	
	for(int i = 0; i<list_size(marcos); i++){
		marco_obtenido = list_get(marcos,i);
		if(marco_obtenido->libre == 1){
			contador++;
		}
	}
	free(marco_obtenido);
	return contador;
}

int cantidad_de_marcos_necesarios(int tamanio){
	return (int) ceil(tamanio/config_valores_memoria.tam_pagina); //PROBAR CASTEO
}

void asignar_marcos_a_proceso(uint32_t pid, int cantidad_de_marcos) {
}

/*
void asignar_marcos_a_proceso(uint32_t pid, int cantidad_de_marcos) {
 	
	t_proceso_en_memoria* proceso = obtener_proceso_en_memoria(pid);
	//if (proceso != -1)...

	int marcos_ya_asignados = cantidad_de_marcos_del_proceso(proceso); //Ver si no choca al ya extender marcos con otros ya cargados

 	t_list_iterator* iterador = list_iterator_create(marcos);

	int marcos_asignando = 0;

 	while (marcos_asignando < cantidad_de_marcos && list_iterator_has_next(iterador)) {
 		t_marco* marco = list_iterator_next(iterador);

		if (marco->libre == 1) {
 			//Actualizo marco
			marco->libre = 0; 
 			marco->pid_proceso = pid; 
 			marco->nro_pag = marcos_ya_asignados; //Aca asigno el nropag a partir de la # de marcos que ya tenia asigandos antes
 			
			// Actualizo paginas 
			t_pagina* pagina = malloc(sizeof(t_pagina));
			pagina->pid = pid;
			pagina->numero_de_pagina = marcos_ya_asignados; //Aca asigno el nropag a partir de la # de marcos que ya tenia asigandos antes sino se pisan los numeros de pag
			pagina->marco = marco->nro_marco;
			pagina->cargada_en_memoria = 1;
			list_add(proceso->paginas_en_memoria, pagina);
			
			//Actualizo contadores
			marcos_asignando++;
			marcos_ya_asignados++;
			}
 	}

 	list_iterator_destroy(iterador);
}



int cantidad_de_marcos_del_proceso(t_proceso_en_memoria* proceso) { //Equivale a cantidad de paginas en memoria

    int cantidad_marcos = 0;
    for (int i = 0; i < list_size(proceso->paginas_en_memoria); i++) {
        t_pagina* pagina = (t_pagina*)list_get(proceso->paginas_en_memoria, i);
        if (pagina->cargada_en_memoria) { 
            cantidad_marcos++;
        }
    }

    return cantidad_marcos; 
}



t_proceso_en_memoria* obtener_proceso_en_memoria(uint32_t pid) { //Busca en los procesos cuyas instrucciones ya fueron cargadas

    for (int i = 0; i < list_size(procesos_en_memoria); i++) {
        t_proceso_en_memoria* proceso = (t_proceso_en_memoria*) list_get(procesos_en_memoria, i);
        if (proceso->pid == pid) {
            return proceso; 
        }
    }

    return -1; 

}

*/
