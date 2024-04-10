#include "io.h"

//static void atender_clientes_io(void*);
static void existe_interfaz(char* nombre);

void iniciar_interfaz(char* nombre, char* path_config) {

    existe_interfaz(nombre);

    t_config* config = config_create(path_config); 

    if(config == NULL) {
        perror("Error al leer el archivo de configuración");
        abort();
    }

    char* tipo_interfaz = config_get_string_value(config, "TIPO_INTERFAZ");

    if (strcmp(tipo_interfaz, "GENERICA") == 0) {
        iniciar_interfaz_generica(nombre, config);
    } else if (strcmp(tipo_interfaz, "STDIN") == 0) {
        iniciar_interfaz_stdin(nombre, config);
    } else if (strcmp(tipo_interfaz, "STDOUT") == 0) {
        iniciar_interfaz_stdout(nombre, config);
    } else if (strcmp(tipo_interfaz, "DIALFS") == 0) {
        iniciar_interfaz_dialfs(nombre, config);
    } else {
        log_error(io_logger, "Tipo de interfaz invalido. Debe ser STDIN, STDOUT, GENERICA o DIALFS");
        abort();
    }
}

static void existe_interfaz(char* nombre) {
    
    int tamanio_lista = list_size(nombres_de_interfaz);

    //Revisamos si ya existe una interfaz con ese nombre
    for(int i = 0; i < tamanio_lista; i++) {
    if(strcmp(list_get(nombres_de_interfaz, i), nombre) == 0) {
        printf("Ya existe una interfaz con ese nombre");
        inicializar_consola_interactiva();
        }
    }

    //Si no existe, la agregamos
    list_add(nombres_de_interfaz, nombre);
}

/*
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
*/