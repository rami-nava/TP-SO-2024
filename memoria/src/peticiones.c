#include "memoria.h"

static void manejo_conexiones(void* conexion);

// ATENDER CLIENTES CON HILOS//
int atender_clientes_memoria(int socket_servidor){

	int* cliente_fd = malloc(sizeof(int));
	*cliente_fd = esperar_cliente(socket_servidor);

	if(*cliente_fd != -1){ //era sin el * pero  eso corrige el warning
		pthread_t hilo_cliente;
		pthread_create(&hilo_cliente, NULL, (void*) manejo_conexiones, cliente_fd);
		pthread_detach(hilo_cliente);
		return 1;
	}else {
		log_error(memoria_logger, "Error al escuchar clientes... Finalizando servidor \n"); 
	}
	return 0;
}

// ATENDER DISPATCH //
static void manejo_conexiones(void* conexion)
{
	int cliente = *(int*)conexion;
	int posicion_pedida = 0;
	int pid_proceso = 0;
	char* path_proceso = NULL;	

	while(1){
		t_paquete* paquete = recibir_paquete(cliente);
		void* stream = paquete->buffer->stream;

		switch(paquete->codigo_operacion){	

		case HANDSHAKE:
			int entero = sacar_entero_de_paquete(&stream);
			enviar_paquete_handshake(cliente);
		break;

		case CREACION_ESTRUCTURAS_MEMORIA:
			pid_proceso = sacar_entero_de_paquete(&stream);
			path_proceso = sacar_cadena_de_paquete(&stream);

			//Abrimos el archivo de instrucciones UNA SOLA vez
			FILE * archivo_instrucciones;
			if (!(archivo_instrucciones = fopen(path_proceso, "r"))) {
				log_error(memoria_logger, "No se encontro el archivo de instrucciones");
			}
			free(path_proceso);

			//Guardo las instrucciones leidas en una lista dentro del proceso
			crear_estructuras_memoria(pid_proceso, archivo_instrucciones);

			int ok_creacion = 1;
			send(cliente, &ok_creacion, sizeof(int), 0);
			log_info(memoria_logger,"Estructuras creadas en memoria exitosamente\n");
		break;
		case MANDAR_INSTRUCCION:
			//la cpu nos manda el program counter y el pid del proceso que recibio para ejecutar
			posicion_pedida = sacar_entero_de_paquete(&stream);
			pid_proceso = sacar_entero_de_paquete(&stream);

			usleep(config_valores_memoria.retardo_respuesta * 1000);  

			//buscamos la instruccion dentro de la lista de instruciones del pid que recibimos 
			char* instruccion_pedida = buscar_instruccion_proceso(posicion_pedida, pid_proceso);
			
			t_paquete* paquete = crear_paquete(INSTRUCCION_SOLICITADA); 
			agregar_cadena_a_paquete(paquete, instruccion_pedida); 
			enviar_paquete(paquete, cliente);
		break;
		case FINALIZAR_EN_MEMORIA:
			int pid = sacar_entero_de_paquete(&stream);
			log_info(memoria_logger,"Recibi pedido de eliminacion de estructuras en memoria\n");
			
			finalizar_en_memoria(pid);

			int ok_finalizacion = 1;
			send(cliente, &ok_finalizacion, sizeof(int), 0);
			log_info(memoria_logger,"Estructuras eliminadas en memoria exitosamente\n");
		break;
		//INSTRUCCIONES DE IO
		case REALIZAR_LECTURA:
			uint32_t direccion_fisica_lectura = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t tamanio_lectura = sacar_entero_sin_signo_de_paquete(&stream);
			realizar_lectura(direccion_fisica_lectura, tamanio_lectura, cliente);
			break;
		case REALIZAR_ESCRITURA:
			uint32_t direccion_fisica_escritura = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t tamanio_escritura = sacar_entero_sin_signo_de_paquete(&stream);
			void* texto_a_guardar = sacar_bytes_de_paquete(&stream, tamanio_escritura);
			realizar_escritura(direccion_fisica_escritura, texto_a_guardar, tamanio_escritura, cliente);
			break;
		case LEER_CONTENIDO_EN_MEMORIA:
			uint32_t cantidad_bytes_a_leer = sacar_entero_de_paquete(&stream);
			uint32_t direccion_fisica_fs_write = sacar_entero_sin_signo_de_paquete(&stream);
			realizar_lectura(direccion_fisica_fs_write, cantidad_bytes_a_leer, cliente);
			break;
		case ESCRIBIR_CONTENIDO_EN_MEMORIA:
			uint32_t cantidad_bytes_a_escribir = sacar_entero_de_paquete(&stream);
			void* contenido = sacar_bytes_de_paquete(&stream, cantidad_bytes_a_escribir);
			uint32_t direccion_fisica_fs_read = sacar_entero_sin_signo_de_paquete(&stream);
			realizar_escritura(direccion_fisica_fs_read, contenido, cantidad_bytes_a_escribir, cliente);
			break;

		case PEDIDO_COPY_STRING:
			break;

		//INSTRUCCIONES DE CPU
		case TRADUCIR_PAGINA_A_MARCO:
			uint32_t numero_pagina = sacar_entero_sin_signo_de_paquete(&stream);
			int pid_mmu = sacar_entero_de_paquete(&stream);
			traducir_pagina_a_marcos(numero_pagina, pid, cliente);
			break;

		case PEDIDO_MOV_IN:
		    uint32_t direccion_fisica = sacar_entero_de_paquete(&stream);
			uint32_t valor_leido = leer_memoria(direccion_fisica);

			send(cliente, &valor_leido, sizeof(uint32_t), 0); // MOV_IN_CPU
			break;

		case PEDIDO_MOV_OUT:
		    uint32_t dir_fisica = sacar_entero_de_paquete(&stream);
			uint32_t valor = sacar_entero_de_paquete(&stream);

			escribir_memoria(dir_fisica, valor);
			break;

		case PEDIDO_RESIZE:
			uint32_t process_id = sacar_entero_de_paquete(&stream);
			uint32_t tamanio = sacar_entero_de_paquete(&stream);
			
			int hay_out_of_memory = out_of_memory(process_id, tamanio); //NOMBRE: out_of_memory

			if(hay_out_of_memory) {
				send(cliente, &hay_out_of_memory, sizeof(int), 0);
			} //DEVOLVER A CPU EL OUT_OF_MEMORY (ENTERO :) )
			else{
				resize(process_id, tamanio);	
			}
			break;

		default:
			break;
		}
		eliminar_paquete(paquete);
	}
}

int out_of_memory(uint32_t pid, uint32_t tamanio){ //SOLO FUNCIONA PARA EXTENDER EL TAMAÑO POR AHORA
	int cantidad_marcos_necesarios = cantidad_de_marcos_necesarios(tamanio);
	int cantidad_marcos_libres = cantidad_de_marcos_libres();

	if(cantidad_marcos_libres >= cantidad_marcos_necesarios)
		return 0;
	else
		return 1; //OUT OF MEMORY
}

void resize(uint32_t pid, uint32_t tamanio){
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

uint32_t tamanio_actual_proceso_en_memoria(uint32_t pid){
	
	t_proceso_en_memoria* proceso = obtener_proceso_en_memoria(pid);
	uint32_t tamanio_actual = list_size(proceso->paginas_en_memoria) * config_valores_memoria.tam_pagina;
	return tamanio_actual;
}


//ESTE METODO NO SIRVE, NO TIENE EN CUENTA QUE PUEDE ESTAR SOBREESCRIBIENDO OTRAS PAGINAS, PERO PARA TESTEAR VA
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

t_marco *marco_desde_df(uint32_t dir_fisica){
	int num_marco = floor(dir_fisica / config_valores_memoria.tam_pagina);
	//pthread_mutex_lock(&mutex_marcos);
	t_marco *marco_elegido = list_get(marcos, num_marco);
	//pthread_mutex_unlock(&mutex_marcos);
	return marco_elegido;
}


//ESTE SOLO SIRVE PARA LEER ENTEROS, PERO RECORDEMOS QUE EN ESTE TP SE PUEDEN LEER MAS DE UN SOLO BYTE
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

//TODO mover estos de leer y escribir a estructuras, y ademas hay que hacerlos para strings