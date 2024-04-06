#include "io.h"

static void atender_clientes_io(void*);

metadata_archivo config_valores_metadata_archivo;
FILE* archivo_fat;

//=======================================================================================================================

void cargar_configuracion(char* path) {

       config = config_create(path); //Leo el archivo de configuracion

      if (config == NULL) {
          perror("Archivo de configuracion de io no encontrado \n");
          abort();
      }

      config_valores_io.ip_io = config_get_string_value(config, "IP_IO");
      config_valores_io.puerto_io = config_get_string_value(config, "PUERTO_IO");
      config_valores_io.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
      config_valores_io.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
      config_valores_io.ip_kernel = config_get_string_value(config, "IP_KERNEL");
      config_valores_io.puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
      config_valores_io.path_base_dialfs = config_get_string_value(config, "PATH_BASE_DIALFS");
      config_valores_io.block_size = config_get_int_value(config, "BLOCK_SIZE");
      config_valores_io.block_count = config_get_int_value(config, "BLOCK_COUNT");
      config_valores_io.tiempo_unidad_de_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_DE_TRABAJO");
      config_valores_io.tipo_interfaz = config_get_string_value(config, "TIPO_INTERFAZ");
}

void protocolo_multihilo() {
     int *cliente_fd = malloc(sizeof(int));
    *cliente_fd = esperar_cliente(server_fd);
    pthread_t multihilo;
    pthread_create(&multihilo, NULL, (void *)atender_clientes_io, cliente_fd);
    pthread_detach(multihilo);
}

static void atender_clientes_io(void* conexion) {
    int cliente_fd = *(int*)conexion;
	
	while (1) 
	{		
		t_paquete* paquete = recibir_paquete(cliente_fd);
		void* stream = paquete->buffer->stream;

		switch(paquete->codigo_operacion)
		{
            //case ALGO:
            //TODO
			//break;

			default:
			break;
		}
		eliminar_paquete(paquete);
	}
}

