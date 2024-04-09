#include "kernel.h"

char* pids; 
int quantum;
//==================================================== ENCOLAR Y DESENCOLAR ====================================================================================
void encolar(t_list *cola, t_pcb *pcb){
    list_add(cola, (void *)pcb);
}

t_pcb *desencolar(t_list *cola){
    return (t_pcb *)list_remove(cola, 0);
}

void agregar_PID(void *valor){
    t_pcb *pcb = (t_pcb *)valor;
    char *pid = string_itoa(pcb->pid);
    
    //pthread_mutex_lock(&mutex_pids);
    string_append_with_format(&pids, " %s ", pid);
    //pthread_mutex_unlock(&mutex_pids);

    free (pid);
}

void listar_PIDS(t_list *cola) {
    list_iterate(cola, agregar_PID);
}

void detener_planificacion() {
          pthread_mutex_lock(&mutex_corriendo);
        while (corriendo == 0) { // Mientras no se detenga
           
            pthread_cond_wait(&cond_corriendo, &mutex_corriendo);
        }
        pthread_mutex_unlock(&mutex_corriendo);
}

void desalojo(){
    t_paquete *paquete = crear_paquete(DESALOJO);
    proceso_en_ejecucion_RR = false;
    agregar_entero_a_paquete(paquete,1);
    enviar_paquete(paquete, socket_cpu_interrupt);
}