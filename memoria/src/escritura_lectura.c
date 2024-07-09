#include "memoria.h"
pthread_mutex_t mutex_espacio_usuario;
//================================================== MANEJO ESCRITURAS EN PAGINAS/MARCOS ==================================================

static bool contenido_cabe_en_marcos(t_proceso_en_memoria* proceso, int tamanio_contenido_bytes);

void escribir_contenido_espacio_usuario(int pid, uint32_t direccion_fisica, uint32_t tamanio_escritura, void* contenido){
	
	t_proceso_en_memoria* proceso = obtener_proceso_en_memoria(pid);

	if(!contenido_cabe_en_marcos(proceso, tamanio_escritura)){
		log_error(memoria_logger, "No se puede leer el contenido solicitado");
        return;
	}

	acceso_a_espacio_usuario(pid, "ESCRITURA EN MEMORIA", direccion_fisica, tamanio_escritura);
	
	pthread_mutex_lock(&mutex_espacio_usuario);
	memcpy(espacio_usuario + direccion_fisica, contenido, tamanio_escritura); 
    pthread_mutex_unlock(&mutex_espacio_usuario);

	free(contenido);
}


void leer_contenido_espacio_usuario(int pid, uint32_t direccion_fisica, uint32_t tamanio_lectura, op_code operacion, int cliente) {
    
	t_proceso_en_memoria* proceso = obtener_proceso_en_memoria(pid);

    // Verificar si el contenido a leer cabe en los marcos del proceso
    if (!contenido_cabe_en_marcos(proceso, tamanio_lectura)){
		log_error(memoria_logger, "No se puede leer el contenido solicitado");
        return;
    }

    // Crear un buffer para almacenar el contenido leído
    void* contenido_leido = malloc(tamanio_lectura);
	
	acceso_a_espacio_usuario(pid, "LECTURA EN MEMORIA", direccion_fisica, tamanio_lectura);

    // Leemos el marco, puede una parte si el offset es mas que 0
	pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(contenido_leido, espacio_usuario + direccion_fisica, tamanio_lectura);
	pthread_mutex_unlock(&mutex_espacio_usuario);

	// Devolver el contenido leído -> según instrucción
	if(operacion == LEER_CONTENIDO_EN_MEMORIA_DESDE_CPU){
		t_paquete* paquete_mov_in = crear_paquete(RESULTADO_MOV_IN);
		agregar_bytes_a_paquete(paquete_mov_in, contenido_leido, tamanio_lectura);
		enviar_paquete(paquete_mov_in, cliente);
	} else if (operacion == LEER_CONTENIDO_EN_MEMORIA_DESDE_STDOUT){
		t_paquete* paquete = crear_paquete(RESULTADO_LECTURA_STDOUT);
		agregar_bytes_a_paquete(paquete, contenido_leido, tamanio_lectura);
		enviar_paquete(paquete, cliente);
	} else {
		t_paquete* paquete_fs_write = crear_paquete(VALOR_LECTURA);
		agregar_bytes_a_paquete(paquete_fs_write, contenido_leido, tamanio_lectura);
		enviar_paquete(paquete_fs_write, cliente);
	}

	free(contenido_leido);
}

static bool contenido_cabe_en_marcos(t_proceso_en_memoria* proceso, int tamanio_contenido_bytes) {
    
	int cantidad_paginas_necesarias = cantidad_de_marcos_necesarios(tamanio_contenido_bytes);

    // Verificar si la cantidad de páginas necesarias cabe en las páginas cargadas en los marcos del proceso
    int cantidad_paginas_cargadas = list_size(proceso->paginas_en_memoria);

	//No importa lo que tengan ya que los procesos van a pisar sus escrituras manejadas con DLS
    return cantidad_paginas_cargadas >= cantidad_paginas_necesarias;
}



