#include "kernel.h"

int indice_pid = 0;
sem_t mutex_pid;
pthread_mutex_t mutex_PROCESOS_DEL_SISTEMA;
t_list* cola_PROCESOS_DEL_SISTEMA;

//==================================================== FUNCIONES ESTATICAS ====================================================================================
static int incrementar_pid();
static void enviar_creacion_estructuras_memoria(int pid, char* path_proceso);


//==================================================== CREAR_PCB ====================================================================================
t_pcb* crear_pcb(char* path) 
{
    t_pcb* nuevo_pcb = malloc(sizeof(t_pcb)); 
    nuevo_pcb->pid = incrementar_pid();
    nuevo_pcb->estado = NEW;
    nuevo_pcb->recursos_asignados = list_create();

    if (path == NULL) {
        log_error(kernel_logger, "No me enviaste un path correcto\n");
        free(nuevo_pcb);
    }

    nuevo_pcb->PC = 0;
    nuevo_pcb->AX = 0;
    nuevo_pcb->BX = 0;
    nuevo_pcb->CX = 0;
    nuevo_pcb->DX = 0;
    nuevo_pcb->EAX = 0;
    nuevo_pcb->EBX = 0;
    nuevo_pcb->ECX = 0;
    nuevo_pcb->EDX = 0;
    nuevo_pcb->SI = 0;
    nuevo_pcb->DI = 0;
    nuevo_pcb->quantum = 0;
    nuevo_pcb->eliminado = 0;
    //nuevo_pcb->direcciones_fisicas = list_create();

    ingresar_a_NEW(nuevo_pcb);
    enviar_creacion_estructuras_memoria(nuevo_pcb->pid, path);

    agregar_proceso_a_lista_procesos_del_sistema(nuevo_pcb);

    sem_post(&hay_procesos_nuevos);
    return nuevo_pcb;
}

static void enviar_creacion_estructuras_memoria(int pid, char* path_proceso){
    t_paquete* paquete = crear_paquete(CREACION_ESTRUCTURAS_MEMORIA);
    agregar_entero_a_paquete(paquete,pid);
    agregar_cadena_a_paquete(paquete,path_proceso);
    enviar_paquete(paquete, socket_memoria);

    int respuesta = 0;
    recv(socket_memoria, &respuesta,sizeof(int), MSG_WAITALL);

    if (respuesta != 1){
        log_error(kernel_logger, "No se pudieron crear estructuras en memoria");
    }
}

static int incrementar_pid() {
    sem_wait(&(mutex_pid));
    indice_pid ++;
    sem_post(&(mutex_pid)); 

    return indice_pid;
}

//==================================================== PROCESAR_PCB ====================================================================================
t_contexto* enviar_a_cpu(t_pcb* proceso) {
    if (contexto_ejecucion != NULL) liberar_memoria_contexto();
	
    iniciar_contexto();

    asignar_PBC_a_contexto(proceso);
    
    enviar_contexto(socket_cpu_dispatch);
    
    recibir_contexto(socket_cpu_dispatch); 
    
    actualizar_PCB(proceso);

    return contexto_ejecucion;
 
}

void actualizar_PCB(t_pcb* proceso){
    proceso->pid = contexto_ejecucion->pid;
    proceso->PC = contexto_ejecucion->PC;
    proceso->AX = contexto_ejecucion->AX;
    proceso->BX = contexto_ejecucion->BX;
    proceso->CX = contexto_ejecucion->CX;
    proceso->DX = contexto_ejecucion->DX;
    proceso->EAX = contexto_ejecucion->EAX;
    proceso->EBX = contexto_ejecucion->EBX;
    proceso->ECX = contexto_ejecucion->ECX;
    proceso->EDX = contexto_ejecucion->EDX;
    proceso->SI = contexto_ejecucion->SI;
    proceso->DI = contexto_ejecucion->DI;
    proceso->quantum = contexto_ejecucion->quantum;
    proceso->eliminado = contexto_ejecucion->eliminado;
    //proceso->direcciones_fisicas = contexto_ejecucion->direcciones_fisicas;
}

void asignar_PBC_a_contexto(t_pcb* proceso){

    contexto_ejecucion->pid = proceso->pid;
    contexto_ejecucion->PC = proceso->PC;
    contexto_ejecucion->AX = proceso->AX;
    contexto_ejecucion->BX = proceso->BX;
    contexto_ejecucion->CX = proceso->CX;
    contexto_ejecucion->DX = proceso->DX;
    contexto_ejecucion->EAX = proceso->EAX;
    contexto_ejecucion->EBX = proceso->EBX;
    contexto_ejecucion->ECX = proceso->ECX;
    contexto_ejecucion->EDX = proceso->EDX;
    contexto_ejecucion->SI = proceso->SI;
    contexto_ejecucion->DI = proceso->DI;
    contexto_ejecucion->quantum = proceso->quantum;
    contexto_ejecucion->eliminado = proceso->eliminado;
    //contexto_ejecucion->direcciones_fisicas = proceso->direcciones_fisicas;
}

void agregar_proceso_a_lista_procesos_del_sistema(t_pcb *proceso){
    pthread_mutex_lock(&mutex_PROCESOS_DEL_SISTEMA);
    encolar(cola_PROCESOS_DEL_SISTEMA, proceso);    
    pthread_mutex_unlock(&mutex_PROCESOS_DEL_SISTEMA);
}

//==================================================== ELIMINAR_PCB ====================================================================================
void liberar_PCB(t_pcb* proceso) {
    
    //liberar_en_memoria(proceso);
    list_destroy_and_destroy_elements(proceso->recursos_asignados, free);
    free(proceso);
}


