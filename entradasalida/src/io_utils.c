#include "io.h"

static t_interfaz* crear_interfaz(char* nombre);

t_config* config;
char* nombre_interfaz;

void iniciar_interfaz(char* nombre, char* path_config) {

    config = config_create(path_config); 

    nombre_interfaz = nombre;

    t_interfaz* interfaz = crear_interfaz(nombre);

    if(config == NULL) {
        perror("Error al leer el archivo de configuración");
        abort();
    }

    char* tipo_interfaz = config_get_string_value(config, "TIPO_INTERFAZ");

    if (strcmp(tipo_interfaz, "GENERICA") == 0) {
        main_generica(interfaz);
    } else if (strcmp(tipo_interfaz, "STDIN") == 0) {
        main_stdin(interfaz);
    } else if (strcmp(tipo_interfaz, "STDOUT") == 0) {
        main_stdout(interfaz);
    } else if (strcmp(tipo_interfaz, "DIALFS") == 0) {
        main_dialfs(interfaz);
    } else {
        perror("Tipo de interfaz invalido. Debe ser STDIN, STDOUT, GENERICA o DIALFS");
        abort();
    }
}

static t_interfaz* crear_interfaz(char* nombre){
    t_interfaz* interfaz_creada = malloc(sizeof(t_interfaz));

    interfaz_creada->nombre_interfaz = nombre;
    interfaz_creada->config_interfaz = config;

    return interfaz_creada;
}

void conectarse_a_kernel(int socket_kernel, op_code codigo_operacion ,char* nombre, char* tipo_interfaz)
{
    t_paquete* paquete = crear_paquete(codigo_operacion);
    agregar_cadena_a_paquete(paquete, nombre);
    agregar_cadena_a_paquete(paquete, tipo_interfaz);
    enviar_paquete(paquete, socket_kernel);
}

void desconectarse(){


    char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char* puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");

    int socket_kernel_desconexion = crear_conexion(ip_kernel, puerto_kernel);

    t_paquete* paquete = crear_paquete(DESCONECTAR_IO);
    agregar_cadena_a_paquete(paquete, nombre_interfaz);
    enviar_paquete(paquete, socket_kernel_desconexion);

    int desconectarme = 0;

    recv(socket_kernel_desconexion, &desconectarme , sizeof(int), MSG_WAITALL);
    
    abort();
}
