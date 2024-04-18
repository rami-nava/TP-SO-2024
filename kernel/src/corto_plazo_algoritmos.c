#include "kernel.h"

pthread_mutex_t proceso_en_ejecucion_RR_mutex;

static t_pcb *proximo_a_ejecutar_FIFO_RR();

void planificador_corto_plazo_segun_algoritmo() {
    char *algoritmo = config_valores_kernel.algoritmo;

    if (!strcmp(algoritmo, "FIFO"))
    {
        planificador_corto_plazo(proximo_a_ejecutar_FIFO_RR);
    }
    else if (!strcmp(algoritmo, "RR"))
    {
        planificador_corto_plazo(proximo_a_ejecutar_FIFO_RR);
    }
    else if (!strcmp(algoritmo, "VRR"))
    {
        //planificador_corto_plazo(proximo_a_ejecutar_VRR);
    }
    else
    {
        log_error(kernel_logger, "Algoritmo invalido. Debe ingresar FIFO o RR o VRR");
        abort();
    }
}

static t_pcb *proximo_a_ejecutar_FIFO_RR(){
    if(list_size(cola_READY) > 0){
        pthread_mutex_lock(&mutex_READY);
        t_pcb *pcb = desencolar(cola_READY);
        pthread_mutex_unlock(&mutex_READY);
        return pcb;
    }else{
        printf("No hay procesos esperando en la cola ready\n");
        return NULL;
    } 
}

 //  Funciones de algoritmo RR //
 void inicializar_reloj_RR(){
    quantum = config_valores_kernel.quantum;
    log_info(kernel_logger, "Se inicializo el hilo para control de quantum");

    pthread_t thread_reloj_RR;
    pthread_create(&thread_reloj_RR,NULL, comenzar_reloj_RR(),NULL);
    pthread_detach(thread_reloj_RR); //Para que kernel siga ejecutando a la par de este hilo
}

void* comenzar_reloj_RR(){
    t_temporal* reloj;

    while(1)
    {
        pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
        if(proceso_en_ejecucion_RR)
        {
        pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
            reloj = temporal_create();
            log_info(kernel_logger,"Comienza el quantum de: %d\n", quantum);

            while(reloj != NULL) //se creo correctamente
            {
                pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
                if(!proceso_en_ejecucion_RR) //hace falta un mutex aca?
                {
                pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
                    //Hubo salida por I/O o Exit
                    log_info(kernel_logger, "Salida por I/O o Exit");
                    temporal_destroy(reloj);
                    reloj = NULL;
                pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
                }
                else if (temporal_gettime(reloj) >= quantum)
                {
                    pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
                    //log_info(kernel_logger, "Desalojando por fin de Quantum\n"); //PID FALTANTE! :(
                    desalojo(1); //Interrumpo la ejecucion por fin de quantum
                    temporal_destroy(reloj);
                    reloj = NULL;
                    pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
                }
                pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
            }
        pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
        }   
        pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);     
    }
}

// Funciones de algoritmo VRR //