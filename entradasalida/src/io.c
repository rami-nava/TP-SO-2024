#include "io.h"

// Variables Globales//
t_log *io_logger;
pthread_t consola;
t_list* nombres_de_interfaz;

int main(void)
{
    io_logger = log_create("/home/utnso/tp-2024-1c-SegmenFault/entradasalida/cfg/io.log", "io.log", 1, LOG_LEVEL_INFO);

    nombres_de_interfaz = list_create();

    pthread_create(&consola, NULL, (void* ) inicializar_consola_interactiva, NULL);

    using_history(); // Inicializar la historia de comando

    pthread_join(consola, NULL);

    return EXIT_SUCCESS;
}
