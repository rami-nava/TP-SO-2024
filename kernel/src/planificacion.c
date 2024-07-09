#include "kernel.h"

bool proceso_en_ejecucion_RR = false;
bool cambio_de_proceso = true;

sem_t hay_procesos_ready;
sem_t hay_procesos_nuevos;
sem_t grado_multiprogramacion;

pthread_mutex_t mutex_NEW;
pthread_mutex_t mutex_READY; 
pthread_mutex_t mutex_AUX_VRR;

t_list *cola_NEW;
t_list *cola_READY;
t_list *cola_AUX_VRR;

t_pcb* proceso_en_ejecucion;

static t_pcb *siguiente_proceso_a_ready();
//========================================================================================================================================

void planificador_largo_plazo(){

    while (1)
    {
        sem_wait(&grado_multiprogramacion);

        sem_wait(&hay_procesos_nuevos);

        detener_planificacion();

        t_pcb *pcb = siguiente_proceso_a_ready();
        
        if(pcb != NULL){
            //el proceso pasa de new a ready
            ingresar_a_READY(pcb); 
        }//else printf("no hay procesos nuevos para ejecutar\n");
    }
} 

void planificador_corto_plazo(t_pcb *(*proximo_a_ejecutar)()){

    crear_colas_bloqueo();

    while (1)
    {
        if(cambio_de_proceso){
            sem_wait(&hay_procesos_ready);

            detener_planificacion();

            //Obtener el proceso a ejecutar
            proceso_en_ejecucion = proximo_a_ejecutar();

            if(proceso_en_ejecucion != NULL){
                estado_proceso anterior = proceso_en_ejecucion->estado;
                proceso_en_ejecucion->estado = EXEC;
                loggear_cambio_de_estado(proceso_en_ejecucion->pid, anterior, proceso_en_ejecucion->estado);
                
                if(!strcmp(config_valores_kernel.algoritmo, "RR") || !strcmp(config_valores_kernel.algoritmo, "VRR"))
                {
                    sem_wait(&rompiendo_reloj); //Esperar hasta que se reinicie el reloj
                    pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
                    proceso_en_ejecucion_RR = true;
                    pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
                }
            } else {
                //printf("nada para ejecutar");
                continue; //Para que vuelva al principio del while
            }
        }

        cambio_de_proceso = true;

        //Enviamos el proceso a ejecutar a la CPU
        contexto_ejecucion = enviar_a_cpu(proceso_en_ejecucion);

        //Recibimos el contexto de ejecucion de la CPU
        recibir_contexto_actualizado(proceso_en_ejecucion, contexto_ejecucion);
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
    if(list_size(cola_NEW) > 0){
        pthread_mutex_lock(&mutex_NEW);
        t_pcb *pcb = desencolar(cola_NEW);
        pthread_mutex_unlock(&mutex_NEW);
        return pcb;
    }
    return NULL;
}

void ingresar_a_READY(t_pcb *pcb)
{
    if(!strcmp(config_valores_kernel.algoritmo, "VRR") && pcb->quantum != 0){
        ingresar_a_AUX_VRR(pcb);
        return;
    }

    //no lo saco de ninguna lista porque no tenemos lista exec
    estado_proceso anterior = pcb->estado;
    pcb->estado = READY;
    loggear_cambio_de_estado(pcb->pid, anterior, pcb->estado);

    pthread_mutex_lock(&mutex_READY);
    encolar(cola_READY, pcb);    
    pthread_mutex_unlock(&mutex_READY);

    log_ingreso_a_ready();

    sem_post(&hay_procesos_ready);
}

void ingresar_a_AUX_VRR(t_pcb *pcb)
{
    estado_proceso anterior = pcb->estado;
    pcb->estado = READY;
    loggear_cambio_de_estado(pcb->pid, anterior, pcb->estado);

    pthread_mutex_lock(&mutex_AUX_VRR);
    encolar(cola_AUX_VRR, pcb);    
    pthread_mutex_unlock(&mutex_AUX_VRR);

    sem_post(&hay_procesos_ready);

    log_ingreso_a_aux_vrr();

    log_ingreso_a_ready();
}

void ingresar_de_BLOCKED_a_READY_IO(t_list* cola, pthread_mutex_t cola_bloqueado_mutex){
    detener_planificacion();
    pthread_mutex_lock(&cola_bloqueado_mutex);
    t_pcb* proceso_desbloqueado = desencolar(cola);
    pthread_mutex_unlock(&cola_bloqueado_mutex);
    ingresar_a_READY(proceso_desbloqueado); 
}

void ingresar_de_BLOCKED_a_READY_recursos(t_pcb* pcb_desbloqueado){
    detener_planificacion();
    ingresar_a_READY(pcb_desbloqueado);
}

void ingresar_a_BLOCKED_recursos(t_pcb *pcb, char* motivo)
{
    cambio_de_proceso = true;

    estado_proceso anterior = pcb->estado;
    pcb->estado = BLOCKED_RECURSO;
    loggear_cambio_de_estado(pcb->pid, anterior, pcb->estado); 

    loggear_motivo_bloqueo(pcb, motivo);
}


void ingresar_a_BLOCKED_IO(t_list* cola, t_pcb *pcb, char* motivo, pthread_mutex_t cola_bloqueado_mutex, char* tipo_interfaz)
{
    cambio_de_proceso = true;

    //Pasa de EXEC a BLOCKED
    estado_proceso anterior = pcb->estado;

    if (!strcmp(tipo_interfaz, "GENERICA")) {
        pcb->estado = BLOCKED_IO_GENERICA;
    } 
    if (!strcmp(tipo_interfaz, "STDIN")) {
        pcb->estado = BLOCKED_IO_STDIN;
    } 
    if (!strcmp(tipo_interfaz, "STDOUT")) {
        pcb->estado = BLOCKED_IO_STDOUT;
    } 
    if (!strcmp(tipo_interfaz, "DIALFS")) {
        pcb->estado = BLOCKED_IO_DIALFS;
    }

    loggear_cambio_de_estado(pcb->pid, anterior, pcb->estado); 

    pthread_mutex_lock(&cola_bloqueado_mutex);
    encolar(cola, pcb); 
    pthread_mutex_unlock(&cola_bloqueado_mutex);

    loggear_motivo_bloqueo(pcb, motivo);
}

void mandar_a_EXIT(t_pcb* pcb_asociado, char* motivo) 
{
    //Saco el proceso de los procesos del sistema
    eliminar_de_cola(cola_PROCESOS_DEL_SISTEMA,pcb_asociado, mutex_PROCESOS_DEL_SISTEMA);

    estado_proceso anterior = pcb_asociado->estado;
   
    sacar_proceso_de_cola_estado_donde_esta(pcb_asociado);

    pcb_asociado->estado = SALIDA;
    loggear_cambio_de_estado(pcb_asociado->pid, anterior, pcb_asociado->estado);

    //Avisas pq finalizo el proceso
    loggear_finalizacion_proceso(pcb_asociado, motivo); 

    //Liberar recursos
    liberar_recursos_asignados(pcb_asociado);

    //Liberamos memoria
    liberar_PCB(pcb_asociado);

    sem_post(&grado_multiprogramacion);
}

