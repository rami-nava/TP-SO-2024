
#include "cpu.h"

arch_config config_valores_cpu;
t_config* config;
int instruccion_actual;
pthread_mutex_t seguir_ejecutando_mutex;
pthread_mutex_t interrupcion_mutex;
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
    pthread_mutex_lock(&seguir_ejecutando_mutex);
    seguir_ejecutando = true;
    pthread_mutex_unlock(&seguir_ejecutando_mutex);

    while(1) 
    {
        t_paquete *paquete = recibir_paquete(socket_cliente_dispatch);
        void *stream = paquete->buffer->stream;

        if (paquete->codigo_operacion == CONTEXTO_ACTUALIZADO) {
				recibir_contexto_cpu(paquete,stream);
                pthread_mutex_lock(&seguir_ejecutando_mutex);
                while(no_es_bloqueante(instruccion_actual) && seguir_ejecutando) {
                    pthread_mutex_unlock(&seguir_ejecutando_mutex); // Revisar si va aca o luego del while
                    ciclo_de_instruccion();
                    pthread_mutex_lock(&seguir_ejecutando_mutex); // Revisar
                }
                pthread_mutex_unlock(&seguir_ejecutando_mutex);	
        }  else {
            perror("No me enviaste el contexto :( \n");
            abort();
            }
    }
}

//================================================== Funciones Auxiliares =====================================================================
bool no_es_bloqueante(codigo_instrucciones instruccion_actual) {
	codigo_instrucciones instrucciones_bloqueantes[13] = {
        WAIT, SIGNAL, EXIT, 
		RESIZE, IO_GEN_SLEEP, IO_STDIN_READ, IO_STDOUT_WRITE, IO_FS_CREATE,
	    IO_FS_DELETE, IO_FS_TRUNCATE, IO_FS_WRITE, IO_FS_READ
        };

        for (int i = 0; i < 13; i++) 
		if (instruccion_actual == instrucciones_bloqueantes[i]) return false;

	return true;
}

//================================================== INICIALIZAR SEMAFOROS =====================================================================
void inicializar_semaforos(){
    pthread_mutex_init(&interrupcion_mutex,NULL);
    pthread_mutex_init(&seguir_ejecutando_mutex,NULL);
}