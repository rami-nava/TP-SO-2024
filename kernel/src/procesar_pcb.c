#include "kernel.h"

int indice_pid = 0;
sem_t mutex_pid;
//==================================================== FUNCIONES ESTATICAS ====================================================================================
static int incrementar_pid();
static void enviar_creacion_estructuras_memoria(t_pcb* pcb);

//==================================================== CREAR_PCB ====================================================================================
t_pcb* crear_pcb(char* path) 
{
    t_pcb* nuevo_pcb = malloc(sizeof(t_pcb)); 
    nuevo_pcb->pid = incrementar_pid();
    nuevo_pcb->estado = NEW;

    //podemos guardar el path en el pcb del proceso directamente
    if (path != NULL) {
    nuevo_pcb->path_proceso = strdup(path);
    } else {
        log_error(kernel_logger, "No me enviaste un path correcto\n");
        free(nuevo_pcb);
    }

    nuevo_pcb->program_counter = 0;
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
    nuevo_pcb->quantum = config_valores_kernel.quantum;

    ingresar_a_NEW(nuevo_pcb);
    enviar_creacion_estructuras_memoria(nuevo_pcb);

    sem_post(&hay_procesos_nuevos);
    return nuevo_pcb;
}

static void enviar_creacion_estructuras_memoria(t_pcb* pcb){
    t_paquete* paquete = crear_paquete(CREACION_ESTRUCTURAS_MEMORIA);
    agregar_entero_a_paquete(paquete,pcb-> pid);
    agregar_cadena_a_paquete(paquete,pcb->path_proceso);
    enviar_paquete(paquete, socket_memoria);

    int respuesta = 0;
    recv(socket_memoria, &respuesta,sizeof(int),0);

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
    proceso->program_counter = contexto_ejecucion->program_counter;
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
}

void asignar_PBC_a_contexto(t_pcb* proceso){

    contexto_ejecucion->pid = proceso->pid;
    contexto_ejecucion->program_counter = proceso->program_counter;
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
}

//==================================================== ELIMINAR_PCB ====================================================================================
void liberar_PCB(t_pcb* proceso) {
    
    //liberar_en_memoria(proceso);
    liberar_memoria_contexto();
}
