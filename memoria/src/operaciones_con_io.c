#include "memoria.h"

static void enviar_lectura(char* lectura, int cliente);

void realizar_lectura(uint32_t direccion_fisica, uint32_t tamanio_lectura, int cliente) 
{
    //TODO
    char* lectura = NULL;
    enviar_lectura(lectura, cliente);    
}

void realizar_escritura(uint32_t direccion_fisica, char* texto_a_guardar) 
{
    //TODO
    //free(texto_a_guardar);
}

static void enviar_lectura(char* lectura, int cliente)
{
    t_paquete* paquete = crear_paquete(DEVOLVER_LECTURA);
	agregar_cadena_a_paquete(paquete, lectura);
	enviar_paquete(paquete, cliente);
}