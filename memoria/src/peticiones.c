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

		//INSTRUCCIONES DE CPU
		case TRADUCIR_PAGINA_A_MARCO:
			uint32_t numero_pagina = sacar_entero_sin_signo_de_paquete(&stream);
			int pid_mmu = sacar_entero_de_paquete(&stream);
			traducir_pagina_a_marcos(numero_pagina, pid_mmu, cliente);
			break;

		case PEDIDO_MOV_IN:
			int pid_mov_in = sacar_entero_de_paquete(&stream);
		    uint32_t direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
			
			uint32_t valor_leido = leer_memoria(direccion_fisica, pid_mov_in);
			send(cliente, &valor_leido, sizeof(uint32_t), 0); 
			break;

		case PEDIDO_MOV_OUT:
			int pid_mov_out = sacar_entero_de_paquete(&stream);
		    uint32_t dir_fisica = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t valor = sacar_entero_sin_signo_de_paquete(&stream);

			escribir_memoria(dir_fisica, valor, pid_mov_out);
			uint32_t se_ha_escrito = 1;
			send(cliente, &se_ha_escrito, sizeof(uint32_t), 0);
			break;

		case PEDIDO_RESIZE:
			int process_id = sacar_entero_de_paquete(&stream);
			uint32_t tamanio = sacar_entero_sin_signo_de_paquete(&stream);
			
			int hay_out_of_memory = out_of_memory(process_id, tamanio); 
			
			//Le enviamos a la cpu si hay o no out of memory
			send(cliente, &hay_out_of_memory, sizeof(int), 0);

			//Si no hay out of memory lo redimensionamos
			if (hay_out_of_memory == 0) resize(process_id, tamanio);

			break;
		case PEDIDO_COPY_STRING:
			int pid_copy_string = sacar_entero_de_paquete(&stream);
			uint32_t cantidad_bytes_a_copiar = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t direccion_fisica_a_copiar = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t direccion_fisica_destino = sacar_entero_sin_signo_de_paquete(&stream);
			copy_string(pid_copy_string, cantidad_bytes_a_copiar, direccion_fisica_a_copiar, direccion_fisica_destino);
			break;
		default:
			break;
		}
		eliminar_paquete(paquete);
	}
}