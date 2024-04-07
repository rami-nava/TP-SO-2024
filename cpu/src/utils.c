
#include "cpu.h"

arch_config config_valores_cpu;
t_config* config;
//================================================== Configuracion =====================================================================

// funcion para levantar el archivo de configuracion de cfg y ponerlo en nuestro stuct de cpu
void cargar_configuracion(char *path)
{
    config = config_create(path);

    if (config == NULL)
    {
        perror("Archivo de configuracion de cpu no encontrado \n");
        abort();
    }

    config_valores_cpu.ip_cpu = config_get_string_value(config, "IP_CPU");
    config_valores_cpu.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    config_valores_cpu.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    config_valores_cpu.puerto_escucha_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    config_valores_cpu.puerto_escucha_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    config_valores_cpu.cantidad_entradas_tlb = config_get_int_value(config, "CANTIDAD_ENTRADAS_TLB");
    config_valores_cpu.algoritmo_tlb = config_get_string_value(config, "ALGORITMO_TLB");
}

//================================================== Servidor KERNEL =====================================================================
void atender_dispatch()
{
    while(1) 
    {
        int instruccion_actual = -1;

        t_paquete *paquete = recibir_paquete(socket_cliente_dispatch);
        void *stream = paquete->buffer->stream;

        if (paquete->codigo_operacion == CONTEXTO_ACTUALIZADO) {
				if (contexto_ejecucion != NULL) 
				recibir_contexto_cpu(paquete,stream);
                while((no_es_bloqueante(instruccion_actual))) {
                    ciclo_de_instruccion();
                }	
        }  else {
            perror("No me enviaste el contexto :( \n");
            abort();
            }
    }
}

//================================================== Funciones Auxiliares =====================================================================


bool no_es_bloqueante(codigo_instrucciones instruccion_actual) {
	codigo_instrucciones instrucciones_bloqueantes[13] = {
		SLEEP, WAIT, SIGNAL, EXIT, 
		F_OPEN, F_CLOSE, F_SEEK, F_READ, F_WRITE, F_TRUNCATE	
        };

        for (int i = 0; i < 13; i++) 
		if (instruccion_actual == instrucciones_bloqueantes[i]) return false;

	return true;
}


