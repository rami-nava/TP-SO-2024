#include "kernel.h"

int quantum;
//==================================================== ENCOLAR Y DESENCOLAR ====================================================================================
void encolar(t_list *cola, t_pcb *pcb){
    list_add(cola, (void *)pcb);
}

t_pcb *desencolar(t_list *cola){
    return (t_pcb *)list_remove(cola, 0);
}

void detener_planificacion() {
          pthread_mutex_lock(&mutex_corriendo);
        while (corriendo == 0) { // Mientras no se detenga
           
            pthread_cond_wait(&cond_corriendo, &mutex_corriendo);
        }
        pthread_mutex_unlock(&mutex_corriendo);
}

void desalojo(int tipo_interrupcion){
    t_paquete *paquete = crear_paquete(DESALOJO);
    pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
    proceso_en_ejecucion_RR = false;
    pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
    agregar_entero_a_paquete(paquete,tipo_interrupcion);
    enviar_paquete(paquete, socket_cpu_interrupt);
}

void mandar_a_EXIT(t_pcb* pcb_asociado, char* motivo) 
{
    cambio_de_estado (pcb_asociado, SALIDA); 

    //Avisas pq finalizo el proceso
    loggear_finalizacion_proceso(pcb_asociado, motivo); 
    
    //si es un error de signal, no quiero mandarlo a liberar un recurso que no existe porque entra en un bucle
    if(strcmp(motivo, "Error de signal, el recurso solicitado no existe") != 0){
        if(!list_is_empty(pcb_asociado->recursos_asignados)) {
            liberar_recursos_asignados(pcb_asociado);
        }
    }

    //Liberamos memoria
    liberar_PCB(pcb_asociado);

    sem_post(&grado_multiprogramacion);
}

void log_ingreso_a_ready() 
{
    if(list_size(cola_READY) > 0){
        mostrar_lista_pcb(cola_READY, "READY", mutex_READY);
    }
}
