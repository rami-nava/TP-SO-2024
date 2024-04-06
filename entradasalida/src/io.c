#include "io.h"

// Variables Globales//
t_log *io_logger;
t_config *config;
int server_fd;
int *cliente_fd;
int socket_memoria;
arch_config config_valores_io;

int tam_bloque;
int tamanio_fat;
int tamanio_archivo_bloques;
char *path_dial_fs;
t_list* procesos_en_io;

t_dictionary *diccionario_archivos_abiertos;

//============================================================================================================

int main(void)
{
    io_logger = log_create("/home/utnso/tp-2024-1c-SegmenFault/io/cfg/io.log", "io.log", 1, LOG_LEVEL_INFO);

    cargar_configuracion("/home/utnso/tp-2024-1c-SegmenFault/io/cfg/io.config");

    // COMUNICACIÓN MEMORIA //
    socket_memoria = crear_conexion(config_valores_io.ip_memoria, config_valores_io.puerto_memoria);

    /// CREA LA CONEXION CON KERNEL Y MEMORIA ///
    int server_fd = iniciar_servidor(config_valores_io.ip_io, config_valores_io.puerto_io);

    tam_bloque = config_valores_io.block_size;
    tamanio_archivo_bloques = config_valores_io.block_size * config_valores_io.block_count;
    path_dial_fs = config_valores_io.path_base_dialfs;

    procesos_en_io = list_create();

    diccionario_archivos_abiertos = dictionary_create();

    //crear_archivo_de_bloque();

    while (1)
    {
       protocolo_multihilo();
    }

    return EXIT_SUCCESS;
}
