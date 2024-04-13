#include "io.h"

t_interfaz* interfaz;
t_config* config; 

static void existe_interfaz(char* nombre);
static t_interfaz* crear_interfaz(char* nombre, t_config* config);
static void iniciar_hilo_de_interfaz(void (*funcion_de_hilo)(t_interfaz*), t_interfaz* interfaz); 

void iniciar_interfaz(char* nombre, char* path_config) {
    config = config_create(path_config); 

    existe_interfaz(nombre);

    if(config == NULL) {
        perror("Error al leer el archivo de configuración");
        abort();
    }

    char* tipo_interfaz = config_get_string_value(config, "TIPO_INTERFAZ");

    if (strcmp(tipo_interfaz, "GENERICA") == 0) {
        iniciar_hilo_de_interfaz(main_generica, interfaz);
    } else if (strcmp(tipo_interfaz, "STDIN") == 0) {
        iniciar_hilo_de_interfaz(main_stdin, interfaz);
    } else if (strcmp(tipo_interfaz, "STDOUT") == 0) {
        iniciar_hilo_de_interfaz(main_stdout, interfaz);
    } else if (strcmp(tipo_interfaz, "DIALFS") == 0) {
        iniciar_hilo_de_interfaz(main_dialfs, interfaz);
    } else {
        log_error(io_logger, "Tipo de interfaz invalido. Debe ser STDIN, STDOUT, GENERICA o DIALFS");
        abort();
    }
}

static void existe_interfaz(char* nombre) {
    
    int tamanio_lista = list_size(interfaces);
    t_interfaz* interfaz_lista;

    //Revisamos si ya existe una interfaz con ese nombre
    for(int i = 0; i < tamanio_lista; i++) {
        interfaz_lista = list_get(interfaces, i);
        if(strcmp(interfaz_lista->nombre_interfaz, nombre) == 0) {
            printf("Ya existe una interfaz con ese nombre\n ");
            inicializar_consola_interactiva();
        }
    }
    
    //Si no existe, la creamos y agregamos a la lista
    interfaz = crear_interfaz(nombre, config);
    list_add(interfaces, interfaz);
}

static t_interfaz* crear_interfaz(char* nombre, t_config* config){
    t_interfaz* interfaz_creada = malloc(sizeof(t_interfaz));

    interfaz_creada->nombre_interfaz = nombre;
    interfaz_creada->config_interfaz = config;

    return interfaz_creada;
}

static void iniciar_hilo_de_interfaz(void (*funcion_de_hilo)(t_interfaz*), t_interfaz* interfaz) 
{
    pthread_t hilo_interfaz;
    pthread_create(&hilo_interfaz, NULL, funcion_de_hilo, interfaz);
    pthread_detach(hilo_interfaz);
}

void conectarse_a_kernel(int socket_kernel, char* nombre, char* tipo_interfaz)
{
    t_paquete* paquete = crear_paquete(INTERFAZ_GENERICA);
    agregar_cadena_a_paquete(paquete, nombre);
    agregar_cadena_a_paquete(paquete, tipo_interfaz);
    enviar_paquete(paquete, socket_kernel);
}
