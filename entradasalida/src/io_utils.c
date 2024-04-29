#include "io.h"

static t_interfaz* crear_interfaz(char* nombre);
static void desconexion_kernel(int socket_kernel_desconexion);
static void desconectar_memoria(char* tipo_interfaz);

t_config* config;
char* nombre_interfaz;
int desconectado_kernel;

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

void desconectarse(){ //TODO que deberia pasar si hay un proceso en IO?

    char* ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char* puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    char* tipo_interfaz = config_get_string_value(config, "TIPO_INTERFAZ");
    
    int socket_kernel_desconexion = crear_conexion(ip_kernel, puerto_kernel);

    if(strcmp(tipo_interfaz, "GENERICA")){ //Si no es generica
        desconectar_memoria(tipo_interfaz);
    }

    /*pthread_t hilo_desconexion_kernel;
    pthread_create(&hilo_desconexion_kernel, NULL, desconexion_kernel, socket_kernel_desconexion);
    pthread_detach(&hilo_desconexion_kernel);
    
    while(desconectado_kernel != 1){}*/

    desconexion_kernel(socket_kernel_desconexion);

    abort();
}

static void desconexion_kernel(int socket_kernel_desconexion){
    t_paquete* paquete = crear_paquete(DESCONECTAR_IO);
    agregar_cadena_a_paquete(paquete, nombre_interfaz);
    enviar_paquete(paquete, socket_kernel_desconexion);

    desconectado_kernel = 0;
    recv(socket_kernel_desconexion, &desconectado_kernel , sizeof(int), MSG_WAITALL);
}

static void desconectar_memoria(char* tipo_interfaz){

    if(!strcmp(tipo_interfaz, "STDIN")){ 
        desconectar_memoria_stdin(tipo_interfaz);
    }
    if(!strcmp(tipo_interfaz, "STDOUT")){
        desconectar_memoria_stdout(tipo_interfaz);
    }
    if(!strcmp(tipo_interfaz, "DIALFS")){ 
        desconectar_memoria_dialfs(tipo_interfaz);
    }
}