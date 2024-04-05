#include "kernel.h"

char* pids; 

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










