#include "memoria.h"


//================================================= Handshake =====================================================================
void enviar_paquete_handshake(int socket_cliente) {

    uint32_t tam_pagina = config_valores_memoria.tam_pagina;

	t_paquete* handshake=crear_paquete(HANDSHAKE);
    agregar_entero_sin_signo_a_paquete(handshake,tam_pagina);

	enviar_paquete(handshake,socket_cliente);
}

//=============================================== INSTRUCCIONES ================================================================
void leer_instrucciones_desde_archivo(t_proceso_en_memoria* proceso, FILE* archivo) 
{
    char *instruccion = NULL;
    size_t longitud = 0;

    while (getline(&instruccion, &longitud, archivo) != -1)
    {
        if (strcmp(instruccion, "\n"))
        {
            int longitud = strlen(instruccion);
            if (instruccion[longitud - 1] == '\n')
                instruccion[longitud - 1] = '\0';

            char* aux = malloc(strlen(instruccion) + 1);
            strcpy(aux, instruccion);
            list_add(proceso->instrucciones, aux);
        }
    }
    free(instruccion);
    fclose(archivo);
}

char* buscar_instruccion_proceso(uint32_t PC, int pid)
{
	//Obtengo el proceso por el pid
	t_proceso_en_memoria *proceso = buscar_proceso(procesos_en_memoria, pid);

	//Busco la instruccion en la lista segun el PC
	char *instruccion = list_get(proceso->instrucciones, PC);
    
	return instruccion;
}

//=============================================== MMU ================================================================

void traducir_pagina_a_marcos(uint32_t numero_pagina, int pid, int cliente)
{
    //Obtengo el marco por el pid y el numero de pagina
    uint32_t marco_pedido = buscar_marco(numero_pagina, pid);

    //Envio el marco a la CPU
    t_paquete* paquete = crear_paquete(NUMERO_MARCO);
    agregar_entero_sin_signo_a_paquete(paquete, marco_pedido);
    enviar_paquete(paquete, cliente);
}

t_marco *marco_desde_df(uint32_t dir_fisica){
	int num_marco = floor(dir_fisica / config_valores_memoria.tam_pagina);
	//pthread_mutex_lock(&mutex_marcos);
	t_marco *marco_elegido = list_get(marcos, num_marco);
	//pthread_mutex_unlock(&mutex_marcos);
	return marco_elegido;
}

///============================================== MOV_IN && MOV_OUT ================================================================

//ESTE SOLO SIRVE PARA LEER ENTEROS, PERO RECORDEMOS QUE EN ESTE TP SE PUEDEN LEER MAS DE UN SOLO BYTE
// MOV_IN
uint32_t leer_memoria(uint32_t dir_fisica)
{
	uint32_t valor_leido = -1;
	//pthread_mutex_lock(&mutex_memoria_usuario);
	memcpy(&valor_leido, espacio_usuario + dir_fisica, sizeof(uint32_t));
	//pthread_mutex_unlock(&mutex_memoria_usuario);

	t_marco *marco = marco_desde_df(dir_fisica);

	sleep(config_valores_memoria.retardo_respuesta / 1000);
	log_info(memoria_logger, "ACCESO A ESPACIO USUARIO - PID [%d] - ACCION: [LEER] - DIRECCION FISICA: [%d]", marco->pid_proceso, dir_fisica); // LOG OBLIGATORIO falta  - Tamaño <TAMAÑO A LEER / ESCRIBIR>

	return valor_leido;
}

//ESTE METODO NO SIRVE, NO TIENE EN CUENTA QUE PUEDE ESTAR SOBREESCRIBIENDO OTRAS PAGINAS, PERO PARA TESTEAR VA
// MOV_OUT
void escribir_memoria(uint32_t dir_fisica, uint32_t valor){

	//pthread_mutex_lock(&mutex_memoria_usuario);
	memcpy(espacio_usuario + dir_fisica, &valor, sizeof(uint32_t));
	//pthread_mutex_unlock(&mutex_memoria_usuario);

	t_marco *marco = marco_desde_df(dir_fisica);

	//TENGO QUE HACER ALGO CON LA PAGINAX????
	sleep(config_valores_memoria.retardo_respuesta / 1000);
	//TERMINAR LOG
	log_info(memoria_logger, "ACCESO A ESPACIO USUARIO - PID [%d] - ACCION: [ESCRIBIR] - DIRECCION FISICA: [%d]", marco->pid_proceso, dir_fisica); // LOG OBLIGATORIO falta  - Tamaño <TAMAÑO A LEER / ESCRIBIR>
}

// Hay que hacerlos para strings

//============================================== Resize ================================================================

void resize(int pid, uint32_t tamanio){
	uint32_t tamanio_actual = tamanio_actual_proceso_en_memoria(pid);
	uint32_t tamanio_reducido;
	int cantidad_paginas_a_sacar;
	int cantidad_marcos_necesarios = cantidad_de_marcos_necesarios(tamanio);

	//RESIZE PARA COMPRIMIR TAMAÑO Y LUEGO PARA EXPANDIR
	if(tamanio<tamanio_actual){
		tamanio_reducido = tamanio_actual-tamanio;

		//ACA TIENE QUE SER PISO
		//Suponiendo pags de 4B
		//Si tiene 20B y el resize es a 16B no pasa nada porque es multiplo 
		//Si tiene 20B y el resize es a 15B -> tamañoreducido=20-15=5
		//Si hay que sacar 5B -> solo se saca 1 pagina, y queda con 16B, sino estariamos sacando de mas y el proceso queda con 12B
		//TODO Hacer una funcion para esto que sea cantmarcosnecesariosextencion y otra cantmarcosnecesarioscompresion
		cantidad_paginas_a_sacar = tamanio_reducido/config_valores_memoria.tam_pagina; 

		quitar_marcos_a_proceso(pid, cantidad_paginas_a_sacar);
	}else{
		asignar_marcos_a_proceso(pid, cantidad_marcos_necesarios); //cant_m_nec equivale a la cant_pags
	}
}

int out_of_memory(int pid, uint32_t tamanio){ //SOLO FUNCIONA PARA EXTENDER EL TAMAÑO POR AHORA
	int cantidad_marcos_necesarios = cantidad_de_marcos_necesarios(tamanio);
	int cantidad_marcos_libres = cantidad_de_marcos_libres();

	if(cantidad_marcos_libres >= cantidad_marcos_necesarios)
		return 0;
	else
		return 1; //OUT OF MEMORY
}

uint32_t tamanio_actual_proceso_en_memoria(int pid){
	
	t_proceso_en_memoria* proceso = obtener_proceso_en_memoria(pid);
	uint32_t tamanio_actual = list_size(proceso->paginas_en_memoria) * config_valores_memoria.tam_pagina;
	return tamanio_actual;
}