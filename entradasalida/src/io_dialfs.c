#include "io.h"

static char *ip_kernel;
static char *puerto_kernel;
static char *ip_memoria;
static char *puerto_memoria;
static int socket_kernel;
static int socket_memoria;
static char *path_dial_fs;
static int tamanio_bloque;
static int cantidad_bloques;
static int tamanio_archivo_bloques;

void main_dialfs(t_interfaz *interfaz_hilo)
{
    char *nombre = interfaz_hilo->nombre_interfaz;
    t_config *config = interfaz_hilo->config_interfaz;

    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    path_dial_fs = config_get_string_value(config, "PATH_DIALFS");
    tamanio_bloque = config_get_int_value(config, "BLOCK_SIZE");
    cantidad_bloques = config_get_int_value(config, "BLOCK_COUNT");

    log_info(io_logger, "Iniciando interfaz DIALFS: %s", nombre);

    tamanio_archivo_bloques = tamanio_bloque * cantidad_bloques;

    socket_kernel = crear_conexion(ip_kernel, puerto_kernel);
    socket_memoria = crear_conexion(ip_memoria, puerto_memoria);

    conectarse_a_kernel(socket_kernel, INTERFAZ_STDOUT,nombre, "DIALFS");

    crear_archivo_de_bloque(path_dial_fs, tamanio_archivo_bloques);

    //recibir_peticiones_de_kernel();
}
/*
void recibir_peticiones_de_kernel()
{
    while (1)
    {
        t_paquete *paquete = recibir_paquete(socket_kernel);
        void *stream = paquete->buffer->stream;

        switch (paquete->codigo_operacion)
        {
        case CREAR_ARCHIVO:
            char* nombre = sacar_cadena_de_paquete(&stream);
            int pid = sacar_entero_de_paquete(&stream);
            log_info(io_logger, "PID: %d - Crear Archivo: %s", pid, nombre);
            crear_archivo(nombre);
            break;
        case LEER_ARCHIVO:
            leer_archivo();
            break;
        case MODIFICAR_ARCHIVO:
            modificar_archivo();
            break;
        case TRUNCAR_ARCHIVO:
            truncar_archivo();
            break;
        case AMPLIAR_ARCHIVO:
            ampliar_archivo();
            break;
        case REDUCIR_ARCHIVO:
            reducir_archivo();
            break;
        case BORRAR_ARCHIVO:
            borrar_archivo();
            break;
        }

        eliminar_paquete(paquete);
    }
}

void crear_archivo(char *nombre_archivo)
{
    char *path_archivo = string_from_format("%s/%s.txt", path_dial_fs, nombre_archivo);

    FILE *archivo = fopen(path_archivo, "w"); 
    fclose(archivo);

    // Creamos la config del archivo nuevo
    t_config *archivo_nuevo = config_create(path_archivo);

    if (archivo_nuevo != NULL)
    {
        // nos guardamos la direccion de memoria del archivos
        uint32_t direccion = archivo;

        config_set_value(archivo_nuevo, "NOMBRE_ARCHIVO", nombre_archivo);
        config_set_value(archivo_nuevo, "BLOQUE_INICIAL", "");
        config_set_value(archivo_nuevo, "TAMANIO_ARCHIVO", "0");
        config_save_in_file(archivo_nuevo, path_archivo);
        log_info(io_logger, "Creamos el config y lo guardamos en disco\n");

        ocupar_un_bloque_del_fs();

        t_paquete *paquete = crear_paquete(ARCHIVO_CREADO);
        agregar_entero_a_paquete(paquete, direccion);
        enviar_paquete(paquete, socket_kernel);
        eliminar_paquete(paquete);

        config_destroy(archivo_nuevo);
        free(path_archivo);
        free(nombre_archivo);
    }
}


void ocupar_un_bloque_del_fs()
{
// TODO
}

void compactacion()
{
// TODO
}*/