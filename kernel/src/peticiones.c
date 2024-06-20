#include "kernel.h"

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
static void a_leer_o_escribir_interfaz(op_code codigo, t_pcb * proceso, t_list* direccion, uint32_t tamanio, t_interfaz* interfaz);
static void a_crear_o_eliminar_archivo(op_code codigo, t_pcb * proceso, char* nombre_archivo, t_interfaz* interfaz);
static void a_truncar_archivo(t_pcb * proceso, uint32_t tamanio, char* nombre_archivo, t_interfaz* interfaz);
static void a_leer_o_escribir_archivo(op_code codigo, t_pcb * proceso, uint32_t puntero, uint32_t tamanio, t_list* direcciones, char* nombre_archivo, t_interfaz* interfaz);
static void recibir_peticion(t_pcb *proceso, t_contexto *contexto_ejecucion);

bool instruccion_bloqueante;

//======================================================================================================================================
void recibir_contexto_actualizado(t_pcb *proceso, t_contexto *contexto_ejecucion)
{
    instruccion_bloqueante = false;
    // Ejecutamos la peticion recibida
    recibir_peticion(proceso, contexto_ejecucion);

    // Verificamos si hay fin de quantum y el proceso no fue bloqueado
    if(contexto_ejecucion->hay_fin_de_quantum && !instruccion_bloqueante)
    {
        cambio_de_proceso = true;
        fin_quantum(proceso);
    }
}

static void recibir_peticion(t_pcb *proceso, t_contexto *contexto_ejecucion){
    switch (contexto_ejecucion->motivo_desalojo->comando){
        case IO_GEN_SLEEP:
            romper_el_reloj();
            io_gen_sleep(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_STDIN_READ:
            romper_el_reloj();
            io_stdin_read(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_STDOUT_WRITE:
            romper_el_reloj();
            io_stdout_write(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_FS_CREATE:
            romper_el_reloj();
            io_fs_create(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_FS_DELETE:
            romper_el_reloj();
            io_fs_delete(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_FS_TRUNCATE:
            romper_el_reloj();
            io_fs_truncate(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_FS_WRITE:
            romper_el_reloj();
            io_fs_write(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case IO_FS_READ:
            romper_el_reloj();
            io_fs_read(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case WAIT:
            wait_s(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case SIGNAL:
            signal_s(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;        
        case EXIT:
            romper_el_reloj();
            exit_c(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
        case OUT_OF_MEMORY:
            exit_c(proceso, contexto_ejecucion->motivo_desalojo->parametros);
            break;
      default:
        break;
    }
}

static void fin_quantum(t_pcb* proceso){
    log_info(kernel_logger, "PID: %d - Desalojado por fin de Quantum\n",proceso_en_ejecucion->pid);
    proceso->quantum = 0;
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
    uint32_t tamanio = atoi(parametros[1]);

    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_leer_o_escribir_interfaz(STDIN_READ, proceso, contexto_ejecucion->direcciones_fisicas, tamanio, interfaz);
    }
}

static void io_stdout_write(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    uint32_t tamanio = atoi(parametros[1]);

    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_leer_o_escribir_interfaz(STDOUT_WRITE, proceso, contexto_ejecucion->direcciones_fisicas, tamanio, interfaz);
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
    uint32_t tamanio = atoi(parametros[2]);
    uint32_t puntero = atoi(parametros[3]);
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_leer_o_escribir_archivo(LEER_ARCHIVO, proceso, puntero, tamanio, contexto_ejecucion->direcciones_fisicas, nombre_archivo, interfaz);
    }
}

static void io_fs_write(t_pcb *proceso, char **parametros){
    char* nombre_interfaz = parametros[0];
    char* nombre_archivo = parametros[1];
    uint32_t tamanio = atoi(parametros[2]);
    uint32_t puntero = atoi(parametros[3]);
    t_interfaz* interfaz = obtener_interfaz_por_nombre(nombre_interfaz);

    bool peticion_valida = peticiones_de_io(proceso, interfaz);

    if (peticion_valida) {
        a_leer_o_escribir_archivo(ESCRIBIR_ARCHIVO, proceso, puntero, tamanio, contexto_ejecucion->direcciones_fisicas, nombre_archivo, interfaz);
    }
}

static void a_mimir(t_pcb * proceso, int tiempo_sleep, t_interfaz* interfaz) 
{      
    t_paquete* paquete = crear_paquete(GENERICA_IO_SLEEP);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_entero_a_paquete(paquete, tiempo_sleep);
    
    enviar_peticion_io(proceso,interfaz,paquete);
}

static void a_leer_o_escribir_interfaz(op_code codigo, t_pcb * proceso, t_list* direcciones, uint32_t tamanio, t_interfaz* interfaz) 
{
    t_paquete* paquete = crear_paquete(codigo);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_lista_de_accesos_a_paquete(paquete, direcciones);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);

    enviar_peticion_io(proceso,interfaz,paquete);
}

static void a_crear_o_eliminar_archivo(op_code codigo, t_pcb * proceso, char* nombre_archivo, t_interfaz* interfaz)
{
    t_paquete* paquete = crear_paquete(codigo);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_a_paquete(paquete, proceso->pid);

    enviar_peticion_io(proceso,interfaz,paquete);
}

static void a_truncar_archivo(t_pcb * proceso, uint32_t tamanio, char* nombre_archivo, t_interfaz* interfaz)
{
    t_paquete* paquete = crear_paquete(TRUNCAR_ARCHIVO);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    agregar_entero_a_paquete(paquete, proceso->pid);

    enviar_peticion_io(proceso,interfaz,paquete);
}

static void a_leer_o_escribir_archivo(op_code codigo, t_pcb * proceso, uint32_t puntero, uint32_t tamanio, t_list* direcciones, char* nombre_archivo, t_interfaz* interfaz)
{
    t_paquete* paquete = crear_paquete(codigo);
    agregar_cadena_a_paquete(paquete, nombre_archivo);
    agregar_entero_sin_signo_a_paquete(paquete, puntero);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    agregar_entero_a_paquete(paquete, proceso->pid);
    agregar_lista_de_accesos_a_paquete(paquete, direcciones);

    enviar_peticion_io(proceso,interfaz,paquete);
}

static void exit_c(t_pcb* proceso, char **parametros){ 
    cambio_de_proceso = true;  
    mandar_a_EXIT(proceso, parametros[0]);
}


