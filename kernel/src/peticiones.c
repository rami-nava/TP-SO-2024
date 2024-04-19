#include "kernel.h"

pthread_mutex_t mutex_FIN_QUANTUM;
pthread_mutex_t mutex_PATOVA;

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
static void a_leer_o_escribir_interfaz(op_code codigo, t_pcb * proceso, uint32_t direccion, uint32_t tamanio, t_interfaz* interfaz);
static void a_crear_o_eliminar_archivo(op_code codigo, t_pcb * proceso, char* nombre_archivo, t_interfaz* interfaz);
static void a_truncar_archivo(t_pcb * proceso, uint32_t tamanio, char* nombre_archivo, t_interfaz* interfaz);
static void a_leer_o_escribir_archivo(op_code codigo, t_pcb * proceso, uint32_t puntero, uint32_t tamanio, uint32_t direccion, char* nombre_archivo, t_interfaz* interfaz);


//======================================================================================================================================

void recibir_contexto_actualizado(t_pcb *proceso, t_contexto *contexto_ejecucion){
    switch (contexto_ejecucion->motivo_desalojo->comando){
        case IO_GEN_SLEEP:
            pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
            proceso_en_ejecucion_RR = false;
            pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
            io_gen_sleep(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_STDIN_READ:
            pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
            proceso_en_ejecucion_RR = false;
            pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
            io_stdin_read(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_STDOUT_WRITE:
            pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
            proceso_en_ejecucion_RR = false;
            pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
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
            pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
            proceso_en_ejecucion_RR = false;
            pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
            if(!strcmp(config_valores_kernel.algoritmo, "RR") || !strcmp(config_valores_kernel.algoritmo, "VRR"))
            {
                sem_wait(&exit_sem);
            }
            exit_c(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case EXIT_MAS_FIN_QUANTUM: //No necesita un sem, ya que el reloj se destruye anteriormente
            pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
            proceso_en_ejecucion_RR = false;
            pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
            exit_c(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case FIN_QUANTUM:
            pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
            proceso_en_ejecucion_RR = false;
            pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
            fin_quantum(proceso);
            break;
    default:
        log_error(kernel_logger, "Comando incorrecto");
        break;
    }
}

static void fin_quantum(t_pcb* proceso){
    pthread_mutex_lock(&mutex_FIN_QUANTUM);
    log_info(kernel_logger, "Fin de quantum del proceso %d\n", proceso->pid);
    proceso->quantum = 0;
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
        a_leer_o_escribir_interfaz(STDIN_READ, proceso, direccion, tamanio, interfaz);
    }
}

static void io_stdout_write(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    uint32_t direccion = atoi(parametros[1]);
    uint32_t tamanio = atoi(parametros[2]);

    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_leer_o_escribir_interfaz(STDOUT_WRITE, proceso, direccion, tamanio, interfaz);
    }
}

static void io_fs_create(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_crear_o_eliminar_archivo(CREAR_ARCHIVO, proceso, nombre_archivo, interfaz);
    }
}

static void io_fs_delete(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_crear_o_eliminar_archivo(ELIMINAR_ARCHIVO, proceso, nombre_archivo, interfaz);
    }
}

static void io_fs_truncate(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    uint32_t tamanio = atoi(parametros[2]);
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_truncar_archivo(proceso, tamanio, nombre_archivo, interfaz);
    }
}

static void io_fs_read(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    uint32_t direccion = atoi(parametros[2]);
    uint32_t tamanio = atoi(parametros[3]);
    uint32_t puntero = atoi(parametros[4]);
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_leer_o_escribir_archivo(LEER_ARCHIVO, proceso, puntero, tamanio, direccion, nombre_archivo, interfaz);
    }
}

static void io_fs_write(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    uint32_t direccion = atoi(parametros[2]);
    uint32_t tamanio = atoi(parametros[3]);
    uint32_t puntero = atoi(parametros[4]);
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_leer_o_escribir_archivo(ESCRIBIR_ARCHIVO, proceso, puntero, tamanio, direccion, nombre_archivo, interfaz);
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

static void a_leer_o_escribir_interfaz(op_code codigo, t_pcb * proceso, uint32_t direccion, uint32_t tamanio, t_interfaz* interfaz) {
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(codigo);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_crear_o_eliminar_archivo(op_code codigo, t_pcb * proceso, char* nombre_archivo, t_interfaz* interfaz){
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(codigo);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_a_paquete(paquete, proceso->pid);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_truncar_archivo(t_pcb * proceso, uint32_t tamanio, char* nombre_archivo, t_interfaz* interfaz){
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(TRUNCAR_ARCHIVO);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    agregar_entero_a_paquete(paquete, proceso->pid);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void a_leer_o_escribir_archivo(op_code codigo, t_pcb * proceso, uint32_t puntero, uint32_t tamanio, uint32_t direccion, char* nombre_archivo, t_interfaz* interfaz){
    int socket_io = interfaz->socket_conectado;

    t_paquete* paquete = crear_paquete(codigo);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_sin_signo_a_paquete(paquete, puntero);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion);
    enviar_paquete(paquete, socket_io);

    crear_hilo_io(proceso, interfaz);
}

static void exit_c(t_pcb* proceso, char **parametros){   

    mandar_a_EXIT(proceso, parametros[0]);
}


