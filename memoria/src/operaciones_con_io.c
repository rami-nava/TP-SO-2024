#include "memoria.h"

static void enviar_lectura(char* lectura, int cliente);

void realizar_escritura(uint32_t direccion_fisica, void* texto_a_guardar, uint32_t tamanio_a_guardar, int cliente) 
{
    log_info(memoria_logger, "Acción: %s - Dirección física: %d ", "ESCRIBIR", direccion_fisica);

	usleep(1000 * config_valores_memoria.retardo_respuesta); 

    char* puntero_a_direccion_fisica = espacio_usuario + direccion_fisica; 

    memcpy(puntero_a_direccion_fisica, texto_a_guardar, tamanio_a_guardar);

    int escritura_completada = 1;
    send(cliente, &escritura_completada, sizeof(int), 0);
 }

void realizar_lectura(uint32_t direccion_fisica, uint32_t cantidad_bytes_a_leer, int cliente) 
{
    log_info(memoria_logger, "Acción: %s - Dirección física: %d ", "LEER", direccion_fisica);

	usleep(1000 * config_valores_memoria.retardo_respuesta); 

	char* puntero_direccion_fisica = espacio_usuario + direccion_fisica; 
    char* lectura = malloc(cantidad_bytes_a_leer);

	memcpy(lectura, puntero_direccion_fisica, cantidad_bytes_a_leer);

    enviar_lectura(lectura, cliente);    
}

static void enviar_lectura(char* lectura, int cliente)
{
    t_paquete* paquete = crear_paquete(DEVOLVER_LECTURA);
    agregar_cadena_a_paquete(paquete, lectura);
	enviar_paquete(paquete, cliente);
}