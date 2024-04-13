#include "io.h"

// Variables Globales//
t_list* interfaces;
t_log* io_logger;

int main(int argc, char* argv[])
{

    if(argc != 3){ //Si no pasan nombre y path del config no se puede inicializar la interfaz
        printf("ERROR: Debe ingresar un nombre y un path al archivo config de la interfaz\n");
        abort();
    }

    char* nombre = argv[1];
    char* path = argv[2];
    
    io_logger = log_create("/home/utnso/tp-2024-1c-SegmenFault/entradasalida/cfg/io.log", "io.log", 1, LOG_LEVEL_INFO);

    interfaces = list_create();

    iniciar_interfaz(nombre, path);

    return EXIT_SUCCESS;
}
