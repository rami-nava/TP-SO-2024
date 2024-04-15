#include "io.h"

t_list* interfaces;

int main(int argc, char* argv[])
{

    if(argc != 3){ //Si no pasan nombre y path del config no se puede inicializar la interfaz
        printf("ERROR: Debe ingresar un nombre y un path al archivo config de la interfaz\n");
        abort();
    }

    char* nombre = argv[1];
    char* path = argv[2];
    
    interfaces = list_create();

    iniciar_interfaz(nombre, path);

    return EXIT_SUCCESS;
}
