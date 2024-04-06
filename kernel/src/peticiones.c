#include "kernel.h"

int tiempo_sleep;

static void sleep_c(t_pcb *proceso, char **parametros);
static void a_mimir(t_pcb * proceso);
static void exit_c(t_pcb* proceso, char **parametros);

static void loggear_motivo_bloqueo(t_pcb* proceso, char* motivo);
static void volver_a_CPU(t_pcb* proceso);


void recibir_contexto_actualizado(t_pcb *proceso, t_contexto *contexto_ejecucion){
    switch (contexto_ejecucion->motivo_desalojo->comando){
        case SLEEP:
            sleep_c(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case INSTRUCCION_EXIT:
            exit_c(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
    default:
        log_error(kernel_logger, "Comando incorrecto");
        break;
    }
}

static void volver_a_CPU(t_pcb* proceso) {
    contexto_ejecucion = enviar_a_cpu(proceso);
    recibir_contexto_actualizado(proceso, contexto_ejecucion);  
}

static void sleep_c(t_pcb *proceso, char **parametros){   
    
    //Puede necesitar mutex
    tiempo_sleep = atoi(parametros[0]);

    cambio_de_estado(proceso, BLOCKED); 

    loggear_motivo_bloqueo(proceso, "IO"); 
          
    pthread_t pcb_bloqueado;

    if (!pthread_create(&pcb_bloqueado, NULL, (void *)a_mimir, (void *)proceso)){
        pthread_detach(pcb_bloqueado);
    } else {
        log_error(kernel_logger,"Error en la creacion de hilo para realizar sleep");
        abort();
    } 
     
}

static void exit_c(t_pcb* proceso, char **parametros){   
    
    //El proceso entra en EXIT
    cambio_de_estado (proceso, EXIT); 

    //Avisas pq finalizo el proceso
    loggear_finalizacion_proceso(proceso, parametros[0]); 
    
    //Liberamos memoria
    liberar_PCB(proceso);

    sem_post(&grado_multiprogramacion); 
}

static void a_mimir(t_pcb * proceso){  
    sleep(tiempo_sleep);
    cambio_de_estado(proceso, READY); 
    ingresar_a_READY(proceso);
}

static void loggear_motivo_bloqueo(t_pcb* proceso, char* motivo) {
    log_info(kernel_logger,"PID: %d - Bloqueado por: %s", proceso->pid, motivo); 
}

void loggear_finalizacion_proceso(t_pcb* proceso, char* motivo) {
    log_info(kernel_logger,"Finaliza el proceso %d - Motivo: %s", proceso->pid, motivo); 
}