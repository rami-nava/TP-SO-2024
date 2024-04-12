#include "kernel.h"

static void io_gen_sleep(t_pcb *proceso, char **parametros);
static void a_mimir(t_pcb * proceso, int tiempo_sleep, t_interfaz* interfaz); 
static void exit_c(t_pcb* proceso, char **parametros);

static void loggear_motivo_bloqueo(t_pcb* proceso, char* motivo);
static void volver_a_CPU(t_pcb* proceso);
static void fin_quantum(t_pcb* proceso);


void recibir_contexto_actualizado(t_pcb *proceso, t_contexto *contexto_ejecucion){
    switch (contexto_ejecucion->motivo_desalojo->comando){
        case IO_GEN_SLEEP:
            io_gen_sleep(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case EXIT:
            exit_c(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case FIN_QUANTUM:
            fin_quantum(proceso);
            break;
    default:
        log_error(kernel_logger, "Comando incorrecto");
        break;
    }
}

static void fin_quantum(t_pcb* proceso){
    ingresar_a_READY(proceso);
}

static void volver_a_CPU(t_pcb* proceso) {
    contexto_ejecucion = enviar_a_cpu(proceso);
    recibir_contexto_actualizado(proceso, contexto_ejecucion);  
}

static void io_gen_sleep(t_pcb *proceso, char **parametros){   
    
    char* nombre_interfaz = parametros[0];
    int tiempo_sleep = atoi(parametros[1]);

    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
    
    a_mimir(proceso, tiempo_sleep, interfaz);

    //Sale de Blocked
    ingresar_a_READY(proceso);

    }
}

static void a_mimir(t_pcb * proceso, int tiempo_sleep, t_interfaz* interfaz) 
{  
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(GENERICA_IO_SLEEP);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_entero_a_paquete(paquete, tiempo_sleep);
    enviar_paquete(paquete, socket_io);

    //Crear funcion ingresar a blocked
    cambio_de_estado(proceso, BLOCKED); 

    loggear_motivo_bloqueo(proceso, "INTERFAZ GENERICA"); 

    int termina_sleep = 0;
    recv(socket_io, &termina_sleep, sizeof(int), 0);
    log_info(kernel_logger, "Finalmente volvio de io el proceso: %d\n", proceso->pid);

}

static void exit_c(t_pcb* proceso, char **parametros){   

    mandar_a_EXIT(proceso, parametros[0]);
}

static void loggear_motivo_bloqueo(t_pcb* proceso, char* motivo) {
    log_info(kernel_logger,"PID: %d - Bloqueado por: %s", proceso->pid, motivo); 
}

void loggear_finalizacion_proceso(t_pcb* proceso, char* motivo) {
    log_info(kernel_logger,"Finaliza el proceso %d - Motivo: %s", proceso->pid, motivo); 
}

