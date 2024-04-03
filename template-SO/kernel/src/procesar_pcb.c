#include "kernel.h"

int indice_pid = 0;
sem_t mutex_pid;
//==================================================== FUNCIONES ESTATICAS ====================================================================================
static int incrementar_pid();

//==================================================== CREAR_PCB ====================================================================================
t_pcb* crear_pcb(char* path) 
{
    t_pcb* nuevo_pcb = malloc(sizeof(t_pcb)); 
    nuevo_pcb->pid = incrementar_pid();
    nuevo_pcb->estado = NEW;
    nuevo_pcb->instrucciones = list_create();
    nuevo_pcb->program_counter = 0;
    nuevo_pcb->AX = 0;
    nuevo_pcb->BX = 0;
    nuevo_pcb->CX = 0;
    nuevo_pcb->DX = 0;

    return nuevo_pcb;
}

static int incrementar_pid() {
    sem_wait(&(mutex_pid));
    indice_pid ++;
    sem_post(&(mutex_pid)); 

    return indice_pid;
}

//==================================================== PROCESAR_PCB ====================================================================================
t_contexto* enviar_a_cpu(t_pcb* proceso) {
    if (contexto_ejecucion != NULL) liberar_memoria_contexto_unico ();
	
    iniciar_contexto();

    asignar_PBC_a_contexto(proceso);
    
    enviar_contexto(socket_cpu_dispatch);

    recibir_contexto(socket_cpu_dispatch); 

    actualizar_PCB(proceso);

    return contexto_ejecucion;
 
}

void actualizar_PCB(t_pcb* proceso){
	list_destroy_and_destroy_elements(proceso->instrucciones, free);
    proceso->instrucciones = list_duplicate(contexto_ejecucion->instrucciones);
    proceso->pid = contexto_ejecucion->pid;
    proceso->program_counter = contexto_ejecucion->program_counter;
    proceso->AX = contexto_ejecucion->AX;
    proceso->BX = contexto_ejecucion->BX;
    proceso->CX = contexto_ejecucion->CX;
    proceso->DX = contexto_ejecucion->DX;
}

void asignar_PBC_a_contexto(t_pcb* proceso){

    list_destroy_and_destroy_elements(contexto_ejecucion->instrucciones, free);
    contexto_ejecucion->instrucciones = list_duplicate(proceso->instrucciones);
    contexto_ejecucion->cantidad_instrucciones = list_size(contexto_ejecucion->instrucciones);
    contexto_ejecucion->pid = proceso->pid;
    contexto_ejecucion->program_counter = proceso->program_counter;
    contexto_ejecucion->AX = proceso->AX;
    contexto_ejecucion->BX = proceso->BX;
    contexto_ejecucion->CX = proceso->CX;
    contexto_ejecucion->DX = proceso->DX;
}

//==================================================== ELIMINAR_PCB ====================================================================================
void liberar_PCB(t_pcb* proceso) {
    
    //liberar_en_memoria(proceso);
    destruir_PCB(proceso); 
    liberar_memoria_contexto_unico();
}

void destruir_PCB(t_pcb *pcb){
    list_destroy_and_destroy_elements(pcb->instrucciones, free);
    free(pcb);
}