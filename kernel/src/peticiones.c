#include "kernel.h"

pthread_mutex_t mutex_FIN_QUANTUM;

//=====================================================================================================================================
static void io_gen_sleep(t_pcb *proceso, char **parametros);
static void io_stdin_read(t_pcb *proceso, char **parametros);
static void io_stdout_write(t_pcb *proceso, char **parametros);
static void io_fs_read(t_pcb *proceso, char **parametros);
static void io_fs_write(t_pcb *proceso, char **parametros);
static void io_fs_delete(t_pcb *proceso, char **parametros);
static void io_fs_truncate(t_pcb *proceso, char **parametros);
static void io_fs_create(t_pcb *proceso, char **parametros);
static void fin_quantum(t_pcb* proceso);
static void exit_c(t_pcb* proceso, char **parametros);

static void a_mimir(t_pcb * proceso, int tiempo_sleep, t_interfaz* interfaz);
static void a_leer_de_interfaz(t_pcb * proceso, uint32_t direccion, uint32_t tamanio, t_interfaz* interfaz);
static void a_escribir_interfaz(t_pcb * proceso, uint32_t direccion, uint32_t tamanio,t_interfaz* interfaz);
static void a_crear_archivo(t_pcb * proceso, char* nombre_archivo, t_interfaz* interfaz);
static void a_eliminar_archivo(t_pcb * proceso, char* nombre_archivo, t_interfaz* interfaz);
static void a_truncar_archivo(t_pcb * proceso, int tamanio_archivo, char* nombre_archivo, t_interfaz* interfaz);
static void a_leer_archivo(t_pcb * proceso, int puntero, int tamanio, int direccion, char* nombre_archivo, t_interfaz* interfaz);
static void a_escribir_archivo(t_pcb * proceso, int puntero, int tamanio, int direccion, char* nombre_archivo, t_interfaz* interfaz);
static void crear_hilo_io(t_pcb* proceso, t_interfaz* interfaz);
static void esperar_io(t_interfaz* interfaz);

//======================================================================================================================================

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
        case IO_FS_CREATE:
            io_fs_create(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_FS_DELETE:
            io_fs_delete(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_FS_TRUNCATE:
            io_fs_truncate(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_FS_WRITE:
            io_fs_write(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_FS_READ:
            io_fs_read(proceso, contexto_ejecucion->motivo_desalojo->parametros);
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
    pthread_mutex_lock(&mutex_FIN_QUANTUM);
    printf("Fin de quantum del proceso %d\n", proceso->pid);
    ingresar_a_READY(proceso);
    pthread_mutex_unlock(&mutex_FIN_QUANTUM);
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
    uint32_t direccion = atoi(parametros[1]);
    uint32_t tamanio = atoi(parametros[2]);

    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_leer_de_interfaz(proceso, direccion, tamanio, interfaz);
    }
}

static void io_stdout_write(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    int direccion = atoi(parametros[1]);
    int tamanio = atoi(parametros[2]);

    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_escribir_interfaz(proceso,direccion, tamanio,interfaz);
    }
}

static void io_fs_create(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_crear_archivo(proceso, nombre_archivo, interfaz);
    }
}

static void io_fs_delete(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_eliminar_archivo(proceso, nombre_archivo, interfaz);
    }
}

static void io_fs_truncate(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    int tamanio = atoi(parametros[2]);
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_truncar_archivo(proceso, tamanio, nombre_archivo, interfaz);
    }
}

static void io_fs_read(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    int direccion = atoi(parametros[2]);
    int tamanio = atoi(parametros[3]);
    int puntero = atoi(parametros[4]);
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_leer_archivo(proceso, puntero, tamanio, direccion, nombre_archivo, interfaz);
    }
}

static void io_fs_write(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    int direccion = atoi(parametros[2]);
    int tamanio = atoi(parametros[3]);
    int puntero = atoi(parametros[4]);
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_escribir_archivo(proceso, puntero, tamanio, direccion, nombre_archivo, interfaz);
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

static void a_leer_de_interfaz(t_pcb * proceso, uint32_t direccion, uint32_t tamanio, t_interfaz* interfaz) {
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(STDIN_READ);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_escribir_interfaz(t_pcb * proceso, uint32_t direccion, uint32_t tamanio,t_interfaz* interfaz){
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(STDOUT_WRITE);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_crear_archivo(t_pcb * proceso, char* nombre_archivo, t_interfaz* interfaz){
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(CREAR_ARCHIVO);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_a_paquete(paquete, proceso->pid);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_eliminar_archivo(t_pcb * proceso, char* nombre_archivo, t_interfaz* interfaz){
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(ELIMINAR_ARCHIVO);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_a_paquete(paquete, proceso->pid);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_truncar_archivo(t_pcb * proceso, int tamanio, char* nombre_archivo, t_interfaz* interfaz){
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(TRUNCAR_ARCHIVO);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_a_paquete(paquete, tamanio);
    agregar_entero_a_paquete(paquete, proceso->pid);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_leer_archivo(t_pcb * proceso, int puntero, int tamanio, int direccion, char* nombre_archivo, t_interfaz* interfaz){
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(LEER_ARCHIVO);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_a_paquete(paquete, puntero);
    agregar_entero_a_paquete(paquete, tamanio);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_entero_a_paquete(paquete, direccion);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_escribir_archivo(t_pcb * proceso, int puntero, int tamanio, int direccion, char* nombre_archivo, t_interfaz* interfaz){
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(ESCRIBIR_ARCHIVO);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_a_paquete(paquete, puntero);
    agregar_entero_a_paquete(paquete, tamanio);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_entero_a_paquete(paquete, direccion);
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
    int pid_io = 0;
    pthread_mutex_lock(&interfaz->comunicacion_interfaz_mutex); //Evita que varios hilos conectados a la misma IO lean el mismo mensaje y ignoren otros
    recv(interfaz->socket_conectado, &pid_io, sizeof(int), 0); 
    pthread_mutex_unlock(&interfaz->comunicacion_interfaz_mutex);
    
    //el proceso pasa de blocked a ready
    ingresar_de_BLOCKED_a_READY(pid_io); 
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

