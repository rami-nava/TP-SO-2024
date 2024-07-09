#include "memoria.h"

static void manejo_conexiones(void* conexion);
static void leer_memoria(int hasta);

// ATENDER CLIENTES CON HILOS//
int atender_clientes_memoria(int socket_servidor){

	int* cliente_fd = malloc(sizeof(int));
	*cliente_fd = esperar_cliente(socket_servidor);

	if(*cliente_fd != -1){ 
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
			finalizar_en_memoria(pid);

			log_info(memoria_logger,"Estructuras eliminadas en memoria exitosamente\n");
		break;

		case TRADUCIR_PAGINA_A_MARCO:
			uint32_t numero_pagina = sacar_entero_sin_signo_de_paquete(&stream);
			int pid_mmu = sacar_entero_de_paquete(&stream);
			traducir_pagina_a_marcos(numero_pagina, pid_mmu, cliente);
			break;

		case LEER_CONTENIDO_EN_MEMORIA_DIALFS:
			int pid_fs_write = sacar_entero_de_paquete(&stream);
			uint32_t cantidad_bytes_a_leer = sacar_entero_de_paquete(&stream);
			uint32_t direccion_fisica_fs_write = sacar_entero_sin_signo_de_paquete(&stream);
			leer_contenido_espacio_usuario(pid_fs_write, direccion_fisica_fs_write, cantidad_bytes_a_leer, LEER_CONTENIDO_EN_MEMORIA_DIALFS, cliente);
			break;
		
		case LEER_CONTENIDO_EN_MEMORIA_DESDE_STDOUT:
			int pid_stdout = sacar_entero_de_paquete(&stream);
			uint32_t bytes_a_leer_stdout = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t direccion_fisica_stdout = sacar_entero_sin_signo_de_paquete(&stream);
			leer_contenido_espacio_usuario(pid_stdout, direccion_fisica_stdout, bytes_a_leer_stdout, LEER_CONTENIDO_EN_MEMORIA_DESDE_STDOUT, cliente);
			break;

		case PEDIDO_RESIZE:
			int process_id = sacar_entero_de_paquete(&stream);
			uint32_t tamanio = sacar_entero_sin_signo_de_paquete(&stream);
			
			int hay_out_of_memory = out_of_memory(process_id, tamanio); 
			
			//Le enviamos a la cpu si hay o no out of memory
			send(cliente, &hay_out_of_memory, sizeof(int), 0);

			//Si no hay out of memory lo redimensionamos
			if (hay_out_of_memory == 0) {
				resize(process_id, tamanio);
			}
			break;

		case PEDIDO_COPY_STRING:
			int pid_copy_string = sacar_entero_de_paquete(&stream);
			uint32_t cantidad_bytes_a_copiar = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t direccion_fisica_a_copiar = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t direccion_fisica_destino = sacar_entero_sin_signo_de_paquete(&stream);
			copy_string(pid_copy_string, cantidad_bytes_a_copiar, direccion_fisica_a_copiar, direccion_fisica_destino);
			break;	

		case DESCONECTAR_IO:
        	sacar_entero_de_paquete(&stream);
			close(cliente);
			break;

		case ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_CPU:
			int pid_escribir_en_memoria_cpu = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t bytes_a_escribir_cpu = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t direccion_fisica_a_escribir_cpu = sacar_entero_sin_signo_de_paquete(&stream);
			void* valor_a_escribir_cpu = sacar_bytes_de_paquete(&stream); 
			escribir_contenido_espacio_usuario(pid_escribir_en_memoria_cpu, direccion_fisica_a_escribir_cpu, bytes_a_escribir_cpu, valor_a_escribir_cpu);
			
			uint32_t escritura_guardada_cpu = 1; 
			send(cliente, &escritura_guardada_cpu, sizeof(uint32_t), 0);
			break;

		case ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_DIALFS:
			int pid_escribir_en_memoria_dialfs = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t bytes_a_escribir_dialfs = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t direccion_fisica_a_escribir_dialfs = sacar_entero_sin_signo_de_paquete(&stream);
			void* valor_a_escribir_dialfs = sacar_bytes_de_paquete(&stream); 
			escribir_contenido_espacio_usuario(pid_escribir_en_memoria_dialfs, direccion_fisica_a_escribir_dialfs, bytes_a_escribir_dialfs, valor_a_escribir_dialfs);
			
			uint32_t escritura_guardada_dialfs = 1; 
			send(cliente, &escritura_guardada_dialfs, sizeof(uint32_t), 0);
			break;

		case ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_STDIN:
			int pid_escribir_en_memoria_stdin = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t bytes_a_escribir_stdin = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t direccion_fisica_a_escribir_stdin = sacar_entero_sin_signo_de_paquete(&stream);
			void* valor_a_escribir_stdin = sacar_bytes_de_paquete(&stream); 
			escribir_contenido_espacio_usuario(pid_escribir_en_memoria_stdin, direccion_fisica_a_escribir_stdin, bytes_a_escribir_stdin, valor_a_escribir_stdin);
			
			uint32_t escritura_guardada_stdin = 1; 
			send(cliente, &escritura_guardada_stdin, sizeof(uint32_t), 0);
			break;

		case LEER_CONTENIDO_EN_MEMORIA_DESDE_CPU:
			int pid_leer_en_memoria_cpu = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t bytes_a_leer_cpu = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t direccion_fisica_a_leer_cpu = sacar_entero_sin_signo_de_paquete(&stream);
			leer_contenido_espacio_usuario(pid_leer_en_memoria_cpu, direccion_fisica_a_leer_cpu, bytes_a_leer_cpu, LEER_CONTENIDO_EN_MEMORIA_DESDE_CPU, cliente);
			break;

		case LEER_MEMORIA:
			int hasta = sacar_entero_de_paquete(&stream);
			leer_memoria(hasta);
			break;

		default:
			break;
		}
		eliminar_paquete(paquete);
	}
}

static void leer_memoria(int hasta){
	mem_hexdump(espacio_usuario, hasta);
}