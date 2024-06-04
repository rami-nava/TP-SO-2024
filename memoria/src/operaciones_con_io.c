#include "memoria.h"

static void enviar_lectura(char* lectura, int cliente);

void realizar_escritura(uint32_t direccion_fisica, void* texto_a_guardar, uint32_t tamanio_a_guardar, int cliente) 
{
    t_marco* marco = marco_desde_df(direccion_fisica);
    char* puntero_a_direccion_fisica = espacio_usuario + direccion_fisica; 

    acceso_a_espacio_usuario(marco->pid_proceso, "ESCRIBIR", direccion_fisica, tamanio_a_guardar);

    memcpy(puntero_a_direccion_fisica, texto_a_guardar, tamanio_a_guardar);

    //mem_hexdump(espacio_usuario, config_valores_memoria.tam_memoria);
    //log_info(memoria_logger, "Se escribio correctamente en la memoria fisica\n");

    int escritura_completada = 1;
    send(cliente, &escritura_completada, sizeof(int), 0);
 }

void realizar_lectura(uint32_t direccion_fisica, uint32_t cantidad_bytes_a_leer, int cliente) 
{
    t_marco* marco = marco_desde_df(direccion_fisica);
    char* puntero_direccion_fisica = espacio_usuario + direccion_fisica; 
    char* lectura = malloc(cantidad_bytes_a_leer);

    acceso_a_espacio_usuario(marco->pid_proceso, "LEER", direccion_fisica, cantidad_bytes_a_leer);

	memcpy(lectura, puntero_direccion_fisica, cantidad_bytes_a_leer);

    //mem_hexdump(espacio_usuario, config_valores_memoria.tam_memoria);

    enviar_lectura(lectura, cliente);    
}

static void enviar_lectura(char* lectura, int cliente)
{
    t_paquete* paquete = crear_paquete(RESULTADO_LECTURA);
    agregar_cadena_a_paquete(paquete, lectura);
	enviar_paquete(paquete, cliente);
}