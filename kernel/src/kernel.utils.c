#include "kernel.h"

//================================================== Configuracion =====================================================================
void cargar_configuracion(char* path) {

    config = config_create(path); 

    if (config == NULL) {
        perror("Archivo de configuracion de kernel no encontrado \n");
        abort();
    }

    config_valores_kernel.puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    config_valores_kernel.ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    config_valores_kernel.puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    config_valores_kernel.ip_cpu = config_get_string_value(config, "IP_CPU");
    config_valores_kernel.puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    config_valores_kernel.puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    config_valores_kernel.algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    config_valores_kernel.quantum = config_get_int_value(config, "QUANTUM");
    config_valores_kernel.recursos = config_get_array_value(config, "RECURSOS");
    config_valores_kernel.instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");
    config_valores_kernel.grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
}

//================================================== SEMAFOROS =====================================================================
void inicializar_semaforos(){   
    pthread_mutex_init(&mutex_NEW, NULL);
    pthread_mutex_init(&mutex_READY,NULL); 
    pthread_mutex_init(&mutex_corriendo, NULL);
    pthread_cond_init(&cond_corriendo, NULL);

  
    sem_init(&hay_procesos_nuevos, 0, 0);
    sem_init(&hay_procesos_ready, 0, 0);
    sem_init(&mutex_pid, 0, 0);
    sem_init(&grado_multiprogramacion, 0, config_valores_kernel.grado_multiprogramacion);
}

//================================================== COLAS =====================================================================
void iniciar_colas(){
    cola_NEW = list_create();
    cola_READY = list_create();
}

void inicializar_planificador(){
    iniciar_colas();
    inicializar_semaforos();
}