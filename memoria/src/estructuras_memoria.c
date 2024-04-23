#include "memoria.h"

void* espacio_usuario;
t_list* procesos_en_memoria;
t_list* marcos;

//============================================ ESPACIO DE USUARIO ===========================================================
void creacion_espacio_usuario(){
	espacio_usuario = malloc(config_valores_memoria.tam_memoria);

	if (espacio_usuario == NULL) {
        perror ("No se pudo alocar memoria al espacio de usuario.");
		//log_error(memoria_logger, "No se pudo alocar memoria al espacio de usuario.");
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
		t_marco* marco = malloc(sizeof(t_marco)); 

		marco->nro_pag = -1;
		marco->nro_marco = i;
		marco->libre = 1;
		marco->pid_proceso = -1;
        
		list_add(marcos, marco);
	}
}


//============================================ FUNCIONES DE LOGICA DE MARCOS/PAGINAS =============================================

int cantidad_de_marcos_libres(){
	
	t_marco* marco_obtenido = NULL;
	int contador=0;
	
	for(int i = 0; i<list_size(marcos); i++){
		marco_obtenido = list_get(marcos,i);
		if(marco_obtenido->libre == 1){
			contador++;
		}
	}
	
	return contador;
}

int cantidad_de_marcos_necesarios(int tamanio){
	return (int) ceil(tamanio/config_valores_memoria.tam_pagina); //PROBAR CASTEO
}

void quitar_marcos_a_proceso(uint32_t pid, uint32_t cantidad_marcos_a_liberar){
	
	t_proceso_en_memoria *proceso = obtener_proceso_en_memoria(pid);
	t_pagina *pagina_removida = NULL;

	for(int i = 0; i<cantidad_marcos_a_liberar; i++){
		
		pagina_removida = list_remove(proceso->paginas_en_memoria, list_size(proceso->paginas_en_memoria)); //Saca desde el final las paginas
		liberar_marco(pagina_removida->marco);

		free(pagina_removida);
	}
	
}

void liberar_marco(int marco_a_liberar){
	
	t_marco *marco = buscar_marco_por_numero(marco_a_liberar);  

	marco->nro_pag = -1;
	marco->libre = 1;
	marco->pid_proceso = -1;
    
}

t_marco* buscar_marco_por_numero(int numero_de_marco) {
    
    for (int i = 0; i < list_size(marcos); i++) {
        t_marco* marco_actual = (t_marco*) list_get(marcos, i);
        
		if (marco_actual->nro_marco == numero_de_marco) {
            
            return marco_actual;
        }
    }
    
    return NULL;
}

void asignar_marcos_a_proceso(uint32_t pid, int cantidad_de_marcos_necesarios) {
	
	//PRIMERO DEBERIA BUSCAR MARCOS LIBRES
	//SEGUNDO AL MARCO LIBRE ASIGNARLE EL PID DEL PROCESO Y MARCARLO COMO OCUPADO 
	//AL PROCESO AGREGARLE UNA PAGINA Y A ESA PAGINA ASIGNARLE EL MARCO Y EL PID
	//SEGUIR RECORRIENDO LA LISTA DE MARCOS LIBRES MIENTRAS CONTADOR SEA MENOR A CANT_MARCOS_NECESARIOS
	
	t_marco* marco_obtenido = NULL;
	int contador=0;

	for(int i = 0; i<list_size(marcos); i++){
		if(contador < cantidad_de_marcos_necesarios){
			marco_obtenido = (t_marco*) list_get(marcos,i);
			
			if(marco_obtenido->libre == 1){ //SI EL MARCO QUE SE OBTIENE ESTA LIBRE
				asignar_proceso_a_marco(pid, marco_obtenido); //TAMBIEN AGREGA PAGINA A PROCESO
				contador++;
			}
		}
	}

	
}

void asignar_proceso_a_marco(uint32_t pid, t_marco* marco){
	t_proceso_en_memoria *proceso = obtener_proceso_en_memoria(pid);
	agregar_pagina_a_proceso(proceso, marco);
	
	marco->pid_proceso = pid;
	marco->nro_pag = list_size(proceso->paginas_en_memoria);
	marco->libre = 0;

	
}

t_proceso_en_memoria* obtener_proceso_en_memoria(uint32_t pid) { //Busca en los procesos cuyas instrucciones ya fueron cargadas
    
	for (int i = 0; i < list_size(procesos_en_memoria); i++) {
        t_proceso_en_memoria* proceso = (t_proceso_en_memoria*) list_get(procesos_en_memoria, i);
        if (proceso->pid == pid) {
            return proceso; 
        }
    }
}

void agregar_pagina_a_proceso(t_proceso_en_memoria* proceso, t_marco* marco){
	
	t_pagina *pagina_nueva = malloc(sizeof(t_pagina));

	pagina_nueva->pid = proceso->pid;
	pagina_nueva->marco = marco->nro_marco;
	pagina_nueva->numero_de_pagina = list_size(proceso->paginas_en_memoria);

	list_add(proceso->paginas_en_memoria, pagina_nueva);
	
}

uint32_t buscar_marco(uint32_t numero_pagina, int pid){
    //Obtengo el proceso por el pid
    t_proceso_en_memoria *proceso = buscar_proceso(procesos_en_memoria, pid);

    // Busco la página correspondiente al número de página
    t_pagina *pagina = NULL;

    for (int i = 0; i < list_size(proceso->paginas_en_memoria); i++) {
        t_pagina *pagina_actual = (t_pagina*) list_get(proceso->paginas_en_memoria, i);
       
	    if (pagina_actual->numero_de_pagina == numero_pagina) {
            pagina = pagina_actual;
            break;
        }
    }

    if (pagina == NULL) {
        log_error(memoria_logger, "No se encontró la página %d en el proceso con PID: %d\n", numero_pagina, pid);
        //return -1; 
    }

    // Obtengo el número de marco asignado a la página
    int marco = pagina->marco;
    if (marco == -1) {
        log_info(memoria_logger, "No hay marco asignado para la página %d en el proceso con PID: %d\n", numero_pagina, pid);
    } else {
        log_info(memoria_logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d\n", pid, numero_pagina, marco);
    }

    
    return marco;
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
        if (pagina->marco != -1) { 
            cantidad_marcos++;
        }
    }

    return cantidad_marcos; 
}
*/

/* // MUESTREA PARA TESTEAR MEMORIA -> NO USAR EN EL TP 
static void mostrar_procesos_en_memoria() {
    printf("=== Procesos en memoria ===\n");
    for (int i = 0; i < list_size(procesos_en_memoria); i++) {
        t_proceso_en_memoria* proceso = (t_proceso_en_memoria*)list_get(procesos_en_memoria, i);
        mostrar_proceso_en_memoria(proceso);
    }
    printf("\n");
}

static void mostrar_marcos() {
    printf("=== Marcos ===\n");
    for (int i = 0; i < list_size(marcos); i++) {
        t_marco* marco = (t_marco*)list_get(marcos, i);
        mostrar_marco(marco);
    }
    printf("\n");
}

static void mostrar_contenido() {
    uint8_t *ptr = (uint8_t*)espacio_usuario;

    for (size_t i = 0; i < config_valores_memoria.tam_memoria; i++) {
        if (ptr[i] >= 32 && ptr[i] <= 126) {
            printf("%c", ptr[i]); // Carácter imprimible
        } else {
            printf("%u", ptr[i]); // Valor numérico
        }
    }
    printf("\n");
}
*/