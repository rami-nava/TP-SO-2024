#include "memoria.h"

//================================================= Handshake =====================================================================
void enviar_paquete_handshake(int socket_cliente) {

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
	t_proceso_en_memoria *proceso = obtener_proceso_en_memoria(pid);

	//Busco la instruccion en la lista segun el PC
	char *instruccion = list_get(proceso->instrucciones, PC);
    
	return instruccion;
}

//=============================================== PEDIDOS MMU ================================================================

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
	int num_marco = numero_marco(dir_fisica);

	//pthread_mutex_lock(&mutex_marcos);
	t_marco *marco_elegido = list_get(marcos, num_marco);
	//pthread_mutex_unlock(&mutex_marcos);

	return marco_elegido;
}



//============================================== Resize ================================================================

void resize(int pid, uint32_t tamanio){
	t_proceso_en_memoria* proceso = obtener_proceso_en_memoria(pid);
	uint32_t tamanio_actual = tamanio_actual_proceso_en_memoria(proceso);
	uint32_t tamanio_reducido;
	int cantidad_paginas_a_sacar;
	int cantidad_marcos_necesarios = cantidad_de_marcos_necesarios(tamanio);

	if(tamanio<tamanio_actual){ //REDUCCION
		
		log_info(memoria_logger, "PID: %d - TAMAÑO ACTUAL: %d - TAMAÑO A REDUCIR: %d", pid, tamanio_actual, tamanio);
		
		tamanio_reducido = tamanio_actual-tamanio;

		//ACA TIENE QUE SER PISO
		//Suponiendo pags de 4B
		//Si tiene 20B y el resize es a 16B no pasa nada porque es multiplo 
		//Si tiene 20B y el resize es a 15B -> tamañoreducido=20-15=5
		//Si hay que sacar 5B -> solo se saca 1 pagina, y queda con 16B, sino estariamos sacando de mas y el proceso queda con 12B
		//TODO Hacer una funcion para esto que sea cantmarcosnecesariosextencion y otra cantmarcosnecesarioscompresion
		cantidad_paginas_a_sacar = tamanio_reducido/tam_pagina; 

		quitar_marcos_a_proceso(proceso, cantidad_paginas_a_sacar);
	}else{ //AUMENTAR TAMAÑO
		log_info(memoria_logger, "PID: %d - TAMAÑO ACTUAL: %d - TAMAÑO A AUMENTAR: %d", pid, tamanio_actual, tamanio);
		asignar_marcos_a_proceso(proceso, cantidad_marcos_necesarios); //cant_m_nec equivale a la cant_pags
	}
}

int out_of_memory(int pid, uint32_t tamanio){ //SOLO FUNCIONA PARA EXTENDER EL TAMAÑO POR AHORA
	int cantidad_marcos_necesarios = cantidad_de_marcos_necesarios(tamanio);
	int cantidad_marcos_libres = cantidad_de_marcos_libres();

	if(cantidad_marcos_libres >= cantidad_marcos_necesarios)
		return 0;
	else {
		log_info(memoria_logger, "OUT OF MEMORY. PID: %d - TAMAÑO: %d", pid, tamanio);
		return 1; 
	}
}

uint32_t tamanio_actual_proceso_en_memoria(t_proceso_en_memoria* proceso){
	
	uint32_t tamanio_actual = list_size(proceso->paginas_en_memoria) * tam_pagina;
	return tamanio_actual;
}

//============================================== Copy String ================================================================

void copy_string(int pid, uint32_t cantidad_bytes_a_copiar, uint32_t direccion_fisica_a_copiar, uint32_t direccion_fisica_destino)
{
	char* puntero_a_direccion_fisica_a_copiar = direccion_fisica_a_copiar + espacio_usuario;
	char* puntero_a_direccion_fisica_destino = direccion_fisica_destino + espacio_usuario;
	char* buffer = malloc(sizeof(char)*cantidad_bytes_a_copiar); 

	acceso_a_espacio_usuario(pid, "LEER", direccion_fisica_a_copiar, cantidad_bytes_a_copiar);

	memcpy(buffer, puntero_a_direccion_fisica_a_copiar, cantidad_bytes_a_copiar);

	acceso_a_espacio_usuario(pid, "ESCRIBIR", direccion_fisica_destino, cantidad_bytes_a_copiar);

	memcpy(puntero_a_direccion_fisica_destino, buffer, cantidad_bytes_a_copiar);

	free(buffer);

	//TODO: Que hacer con páginas? Se copia correctamente el string?
}

