#include "memoria.h"

int cantidad_paginas_proceso;
char* path_recibido = NULL;
int socket_fs;
size_t tamanio_contenido;
int pid_fs;
pthread_mutex_t mutex_path;
pthread_mutex_t mutex_instrucciones;
pthread_mutex_t mutex_lista_instrucciones;

static void manejo_conexiones(void* conexion);


// ATENDER CLIENTES CON HILOS//
int atender_clientes_memoria(int socket_servidor){

	   int* cliente_fd = malloc(sizeof(int));
        *cliente_fd = esperar_cliente(socket_servidor);

	if(cliente_fd != -1){
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
	int pid_proceso_escribir = 0;
	int pid_proceso_leer = 0;
	int* valor_registro = 0;
	uint32_t direccion_fisica = 0;
	uint32_t direccion_logica = 0;
	//char* path_asignado = NULL;
	char* path_proceso = NULL;
	uint32_t numero_pagina;
	uint32_t tam_contenido;
	uint32_t puntero_de_archivo;
	char* nombre_archivo;
	void* contenido = NULL;

	while(1){
	t_paquete* paquete = recibir_paquete(cliente);
    void* stream = paquete->buffer->stream;

	switch(paquete->codigo_operacion){		
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

	case CREACION_ESTRUCTURAS_MEMORIA:
		pid_proceso = sacar_entero_de_paquete(&stream);
		path_proceso = sacar_cadena_de_paquete(&stream);

		//abrimos el archivo de instrucciones del proceso 1 sola vez a penas los creamos
		FILE * archivo_instrucciones;
		if (!(archivo_instrucciones = fopen(path_proceso, "r"))) {
			log_error(memoria_logger, "No se encontro el archivo de instrucciones");
			return -1;
		}
		free(path_proceso);

		//guardo las instrucciones leidas en una lista dentro del proceso
		t_proceso_en_memoria* proceso_nuevo = crear_estructuras_memoria(pid_proceso, archivo_instrucciones);

		int ok_creacion = 1;
        send(cliente, &ok_creacion, sizeof(int), 0);
		log_info(memoria_logger,"Estructuras creadas en memoria exitosamente\n");
		break;

	case FINALIZAR_EN_MEMORIA:
		int pid = sacar_entero_de_paquete(&stream);
		log_info(memoria_logger,"Recibi pedido de eliminacion de estructuras en memoria\n");
		//finalizar_en_memoria(pid);

	    int ok_finalizacion = 1;
        send(cliente, &ok_finalizacion, sizeof(int), 0);
		log_info(memoria_logger,"Estructuras eliminadas en memoria exitosamente\n");
		break;

	default:
		break;
	}
	eliminar_paquete(paquete);
	}
}

