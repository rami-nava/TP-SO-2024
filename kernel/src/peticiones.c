#include "kernel.h"

static void io_gen_sleep(t_pcb *proceso, char **parametros);
static void io_stdin_read(t_pcb *proceso, char **parametros);
static void io_stdout_write(t_pcb *proceso, char **parametros);
static void a_mimir(t_pcb * proceso, int tiempo_sleep, t_interfaz* interfaz);
static void a_leer_de_interfaz(t_pcb * proceso, int direccion, int tamanio, t_interfaz* interfaz);
static void a_escribir_interfaz(t_pcb * proceso, char* nombre_archivo, int direccion, int tamanio, int puntero_archivo,t_interfaz* interfaz);
static void exit_c(t_pcb* proceso, char **parametros);

void loggear_motivo_bloqueo(t_pcb* proceso, char* motivo);
static void fin_quantum(t_pcb* proceso);
static void crear_hilo_io(t_pcb* proceso, t_interfaz* interfaz);
static void esperar_io(t_interfaz* interfaz);


void recibir_contexto_actualizado(t_pcb *proceso, t_contexto *contexto_ejecucion){
    switch (contexto_ejecucion->motivo_desalojo->comando){
        case IO_GEN_SLEEP:
            io_gen_sleep(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_STDIN_READ:
            io_stdin_read(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_STDOUT_WRITE:
            io_stdout_write(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case WAIT:
            wait_s(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case SIGNAL:
            signal_s(proceso, contexto_ejecucion->motivo_desalojo->parametros);
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

void volver_a_CPU(t_pcb* proceso) {
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
    }
}

static void io_stdin_read(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    int direccion = atoi(parametros[1]);
    int tamanio = atoi(parametros[2]);

    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_leer_de_interfaz(proceso, direccion, tamanio, interfaz);
    }
}

static void io_stdout_write(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    int direccion = atoi(parametros[2]);
    int tamanio = atoi(parametros[3]);
    int puntero_archivo = atoi(parametros[4]);

    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_escribir_interfaz(proceso, nombre_archivo,direccion, tamanio, puntero_archivo,interfaz);
    }
}

static void a_mimir(t_pcb * proceso, int tiempo_sleep, t_interfaz* interfaz) 
{  
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(GENERICA_IO_SLEEP);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_entero_a_paquete(paquete, tiempo_sleep);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_leer_de_interfaz(t_pcb * proceso, int direccion, int tamanio, t_interfaz* interfaz) {
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(GENERICA_IO_SLEEP);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_entero_a_paquete(paquete, direccion);
    agregar_entero_a_paquete(paquete, tamanio);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_escribir_interfaz(t_pcb * proceso, char* nombre_archivo, int direccion, int tamanio, int puntero_archivo,t_interfaz* interfaz){
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(GENERICA_IO_SLEEP);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_a_paquete(paquete, direccion);
    agregar_entero_a_paquete(paquete, tamanio);
    agregar_entero_a_paquete(paquete, puntero_archivo);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void crear_hilo_io(t_pcb* proceso, t_interfaz* interfaz){
    char motivo[20] = "";

    strcat(motivo, "INTERFAZ ");
    strcat(motivo, interfaz->tipo_interfaz);

    ingresar_a_BLOCKED(proceso, motivo);

    pthread_t hilo_manejo_io;
    pthread_create(&hilo_manejo_io, NULL, (void* ) esperar_io, interfaz);
    pthread_detach(hilo_manejo_io);
}

static void esperar_io(t_interfaz* interfaz){
    int termina_io = 0;
    pthread_mutex_lock(&interfaz->comunicacion_interfaz_mutex); //Evita que varios hilos conectados a la misma IO lean el mismo mensaje y ignoren otros
    recv(interfaz->socket_conectado, &termina_io, sizeof(int), 0); 
    pthread_mutex_unlock(&interfaz->comunicacion_interfaz_mutex);
    
    //Sale de Blocked
    ingresar_a_READY(desencolar(cola_BLOCKED)); //En realidad habria que buscar el proceso que se acaba de desbloquear

    printf("Se volvio de IO \n");
}

static void exit_c(t_pcb* proceso, char **parametros){   

    mandar_a_EXIT(proceso, parametros[0]);
}

void loggear_motivo_bloqueo(t_pcb* proceso, char* motivo) {
    log_info(kernel_logger,"PID: %d - Bloqueado por: %s\n", proceso->pid, motivo); 
}

void loggear_finalizacion_proceso(t_pcb* proceso, char* motivo) {
    log_info(kernel_logger,"Finaliza el proceso %d - Motivo: %s\n", proceso->pid, motivo); 
}

