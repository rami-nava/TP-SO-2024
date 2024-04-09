#include "kernel.h"

sem_t hay_procesos_ready;
sem_t hay_procesos_nuevos;
sem_t grado_multiprogramacion;

pthread_mutex_t mutex_NEW;
pthread_mutex_t mutex_READY; 

t_list *cola_NEW;
t_list *cola_READY;

char *estados_procesos[5] = {"NEW", "READY", "EXEC", "BLOCK", "SALIDA"};


static t_pcb *siguiente_proceso_a_ready();
static void log_ingreso_a_ready();
//========================================================================================================================================

void planificador_largo_plazo(){

    while (1)
    {
        sem_wait(&hay_procesos_nuevos);

        sem_wait(&grado_multiprogramacion);

        detener_planificacion();

        t_pcb *pcb = siguiente_proceso_a_ready();

        cambio_de_estado(pcb, READY);
        
        ingresar_a_READY(pcb); 

    }
} 

void planificador_corto_plazo(t_pcb *(*proximo_a_ejecutar)()){

    while (1)
    {
        sem_wait(&hay_procesos_ready);

        detener_planificacion();

        //Obtener el proceso a ejecutar
        t_pcb *proceso = proximo_a_ejecutar();
        
        //Loggear estado 
        cambio_de_estado(proceso, EXEC);
        
        //Enviamos el proceso a ejecutar a la CPU
        contexto_ejecucion = enviar_a_cpu(proceso);

        //Recibimos el contexto de ejecucion de la CPU
        recibir_contexto_actualizado(proceso, contexto_ejecucion);

    }
}

void ingresar_a_NEW(t_pcb *pcb)
{
    pthread_mutex_lock(&mutex_NEW);
    encolar(cola_NEW, pcb);
    log_info(kernel_logger, "Se crea el proceso %d en NEW \n", pcb->pid);
    pthread_mutex_unlock(&mutex_NEW);
}

static t_pcb *siguiente_proceso_a_ready()
{
    pthread_mutex_lock(&mutex_NEW);
    t_pcb *pcb = desencolar(cola_NEW);
    pthread_mutex_unlock(&mutex_NEW);
    return pcb;
}

void ingresar_a_READY(t_pcb *pcb)
{
    pthread_mutex_lock(&mutex_READY);
    encolar(cola_READY, pcb);    
    pthread_mutex_unlock(&mutex_READY);

    sem_post(&hay_procesos_ready);

    log_ingreso_a_ready();
   
}

//=====================================================LOGS MINIMOS Y OBLIGATORIOS==================================================================================//
void cambio_de_estado (t_pcb *pcb, estado_proceso estado_nuevo) 
{
    //Estado anterior
    estado_proceso anterior = pcb->estado;

    //Cambiar estado
    pcb->estado = estado_nuevo;

    //Loggear cambio de estado
    log_info(kernel_logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s \n", pcb->pid, estados_procesos[anterior], estados_procesos[estado_nuevo]);

}

static void log_ingreso_a_ready() 
{
    pids = string_new();
    listar_PIDS(cola_READY);

    //pthread_mutex_lock(&mutex_pids);
    log_info(kernel_logger, "Cola Ready %s: %s \n", config_valores_kernel.algoritmo, pids);
    //pthread_mutex_unlock(&mutex_pids);

    free(pids);
}

