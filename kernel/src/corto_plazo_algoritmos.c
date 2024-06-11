#include "kernel.h"

pthread_mutex_t proceso_en_ejecucion_RR_mutex;
sem_t ciclo_actual_quantum_sem;
sem_t rompiendo_reloj;
sem_t exit_sem;

static int quantum_total;
int ciclo_actual_quantum = 0;

static t_pcb *proximo_a_ejecutar_FIFO_RR();
static t_pcb *proximo_a_ejecutar_VRR();

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
        planificador_corto_plazo(proximo_a_ejecutar_VRR);
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

static t_pcb *proximo_a_ejecutar_VRR(){
    if(list_size(cola_AUX_VRR) > 0){
        pthread_mutex_lock(&mutex_AUX_VRR);
        t_pcb *pcb = desencolar(cola_AUX_VRR);
        pthread_mutex_unlock(&mutex_AUX_VRR);
        return pcb;
    }else{
        pthread_mutex_lock(&mutex_READY);
        t_pcb* pcb = desencolar(cola_READY);
        pthread_mutex_unlock(&mutex_READY);
        return pcb;
    }
}

 //  Funciones de algoritmo RR //
 void inicializar_reloj_RR(){
    log_info(kernel_logger, "Se inicializo el hilo para control de quantum");

    pthread_t thread_reloj_RR;
    pthread_create(&thread_reloj_RR,NULL, comenzar_reloj_RR(),NULL);
    pthread_detach(thread_reloj_RR); //Para que kernel siga ejecutando a la par de este hilo
}

void* comenzar_reloj_RR(){
    t_temporal* reloj;

    while(1)
    {
        quantum_total = config_valores_kernel.quantum;
        pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
        if(proceso_en_ejecucion_RR)
        {
        pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
            reloj = temporal_create();
            if(proceso_en_ejecucion->quantum != 0){
                quantum_total -= proceso_en_ejecucion->quantum;
                log_info(kernel_logger,"Quantum a finalizar: %d", quantum_total);
            }else{
                log_info(kernel_logger,"Comienza el quantum de: %d", quantum_total);
            }
            while(reloj != NULL) //se creo correctamente
            {
                pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
                if(!proceso_en_ejecucion_RR) 
                {
                pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);

                    //Hubo salida por I/O o Exit
                     if(!strcmp(config_valores_kernel.algoritmo, "VRR") && ocurrio_IO(contexto_ejecucion)){ 
                        //el ocurrio_IO para darle prioridad a los IO BOUND y no a aquellos que hagan WAIT por ejemplo
                        ciclo_actual_quantum = temporal_gettime(reloj);
                        sem_post(&ciclo_actual_quantum_sem); //Si no es exit no habria que hacer post
                    }

                    log_info(kernel_logger, "Salida por I/O o Exit");
                    temporal_destroy(reloj);
                    reloj = NULL;

                    if(contexto_ejecucion->motivo_desalojo->comando == EXIT){
                        sem_post(&exit_sem); //Para evitar condiciones de carrera y se pueda reiniciar el quantum
                    }

                    //Avisar que ya rompi el reloj antes de iniciar un nuevo proceso
                    sem_post(&rompiendo_reloj);

                pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
                }
                else if (temporal_gettime(reloj) >= quantum_total)
                {

                    ciclo_actual_quantum = 0;
                    sem_post(&ciclo_actual_quantum_sem); //Para que no se bloquee si hay FQ y IO

                    pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
                    desalojo(1); //Interrumpo la ejecucion por fin de quantum
                    temporal_destroy(reloj);
                    reloj = NULL;

                    //Avisar que ya rompi el reloj antes de iniciar un nuevo proceso
                    sem_post(&rompiendo_reloj);

                    pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
                }
                pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
            }
        pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
        }   
        pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);     
    }
}

// Es cuando desalojan el proceso sin que haya fin de Quantum
void romper_el_reloj()
{
    pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
    proceso_en_ejecucion_RR = false;
    pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);

    instruccion_bloqueante = true;
}