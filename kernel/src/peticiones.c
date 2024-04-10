#include "kernel.h"

int tiempo_sleep;

static void sleep_c(t_pcb *proceso, char **parametros);
static void a_mimir(t_pcb * proceso);
static void exit_c(t_pcb* proceso, char **parametros);

static void loggear_motivo_bloqueo(t_pcb* proceso, char* motivo);
static void volver_a_CPU(t_pcb* proceso);


void recibir_contexto_actualizado(t_pcb *proceso, t_contexto *contexto_ejecucion){
    switch (contexto_ejecucion->motivo_desalojo->comando){
        case IO_GEN_SLEEP:
            sleep_c(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case EXIT:
            exit_c(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
    default:
        log_error(kernel_logger, "Comando incorrecto");
        break;
    }
}
/*
void peticiones_de_io(proceso, parametros) {
    if (!conectada_y_existe_interfaz(parametros[0])) {
        log_error(kernel_logger, "Interfaz inexistente");
        mandar_a_EXIT(proceso, "ERROR");
    }

    if(!admite_operacion(parametros[0], contexto_ejecucion->motivo_desalojo->comando))
    {
        log_error(kernel_logger, "Interfaz incorrecta");
        mandar_a_EXIT(proceso, "ERROR");
    }

    t_paquete* paquete = crear_paquete(OPERACION_IO);
    agregar_cadena_a_paquete(paquete, parametros[0]);
    agregar_cadena_a_paquete(paquete, parametros[1]);
    enviar_paquete(paquete, socket_io);
}

bool conectada_y_existe_interfaz(char* interfaz) {

    if (strcmp(interfaz, "GENERICA") == 0 || strcmp(interfaz, "STDOUT") == 0 || 
    strcmp(interfaz, "STDIN") == 0 || strcmp(interfaz, "DIALFS") == 0) {
        return esta_conectado(socket_io);
    }
    
    return false;
}

bool admite_operacion(char* interfaz, char* operacion) {
    if (strcmp(interfaz, "GENERICA") == 0) {
        return strcmp(operacion, "IO_GEN_SLEEP") == 0;
    } 
    if (strcmp(interfaz, "STDIN") == 0) {
        return strcmp(operacion, "IO_STDIN_READ") == 0;
    } 
    if (strcmp(interfaz, "STDOUT") == 0) {
        return strcmp(operacion, "IO_STDOUT_WRITE") == 0;
    } 
    if (strcmp(interfaz, "DIALFS") == 0) {
        return strcmp(operacion, "IO_FS_CREATE") == 0 || strcmp(operacion, "IO_FS_WRITE") == 0 || strcmp(operacion, "IO_FS_READ") == 0; || strcmp(operacion, "IO_FS_DELETE") == 0 || strcmp(operacion, "IO_FS_TRUNCATE") == 0;
    }
    return false;
}
*/
static void volver_a_CPU(t_pcb* proceso) {
    contexto_ejecucion = enviar_a_cpu(proceso);
    recibir_contexto_actualizado(proceso, contexto_ejecucion);  
}

static void sleep_c(t_pcb *proceso, char **parametros){   
    
    //Puede necesitar mutex
    tiempo_sleep = atoi(parametros[0]);

    cambio_de_estado(proceso, BLOCKED); 

    loggear_motivo_bloqueo(proceso, "INTERFAZ GENERICA"); 
          
    pthread_t pcb_bloqueado;

    if (!pthread_create(&pcb_bloqueado, NULL, (void *)a_mimir, (void *)proceso)){
        pthread_detach(pcb_bloqueado);
    } else {
        log_error(kernel_logger,"Error en la creacion de hilo para realizar sleep");
        abort();
    } 
     
}

static void exit_c(t_pcb* proceso, char **parametros){   

    mandar_a_EXIT(proceso, parametros[0]);
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

