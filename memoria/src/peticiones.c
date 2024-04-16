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
			uint32_t direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
			uint32_t tamanio_lectura = sacar_entero_sin_signo_de_paquete(&stream);

			char* lectura = realizar_lectura(direccion_fisica, tamanio_lectura);

			t_paquete* paquete = crear_paquete(DEVOLVER_LECTURA);
			agregar_cadena_a_paquete(paquete, lectura);
			enviar_paquete(paquete, cliente);
			break;
		case REALIZAR_ESCRITURA:
			uint32_t direccion_fisica = sacar_entero_sin_signo_de_paquete(&stream);
			char* texto_a_guardar = sacar_cadena_de_paquete(&stream);
			realizar_escritura(direccion_fisica, texto_a_guardar);

			int escritura_guardada = 1;
			send(cliente, &escritura_guardada, sizeof(int), 0);
			break;
		
		//INSTRUCCIONES DE CPU
		case PEDIDO_MOV_IN:
			break;
		case PEDIDO_MOV_OUT:
			break;
		case PEDIDO_RESIZE:
			break;
		case PEDIDO_COPY_STRING:
			break;
		case PEDIDO_IO_STDIN_READ:
			break;
		case PEDIDO_IO_STDOUT_WRITE:
			break;
		case PEDIDO_IO_FS_WRITE:
			break;
		case PEDIDO_IO_FS_READ:
			break;
		default:
			break;
		}
		eliminar_paquete(paquete);
	}
}
