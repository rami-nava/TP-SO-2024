#include "memoria.h"

static void enviar_lectura(char* lectura, int cantidad_bytes_a_leer, int cliente);

void realizar_lectura(uint32_t direccion_fisica, uint32_t cantidad_bytes_a_leer, int cliente) 
{
    //TODO
    void* lectura = NULL;
    enviar_lectura(lectura, cantidad_bytes_a_leer, cliente);    
}

void realizar_escritura(uint32_t direccion_fisica, char* texto_a_guardar, uint32_t tamanio_a_guardar) 
{
    //Falta cantidad_de_bytes?
    //TODO
    //free(texto_a_guardar);
}

static void enviar_lectura(char* lectura, int cantidad_bytes_a_leer, int cliente)
{
    t_paquete* paquete = crear_paquete(DEVOLVER_LECTURA);
    agregar_bytes_a_paquete(paquete, lectura, cantidad_bytes_a_leer);
	enviar_paquete(paquete, cliente);
}