#include "cpu.h"

// Variables globales //

char *comandos[] = {
    [SET] = "SET",
    [MOV_IN] = "MOV_IN",
    [MOV_OUT] = "MOV_OUT", 
    [SLEEP] = "SLEEP",
    [WAIT] = "WAIT",
    [SIGNAL] = "SIGNAL",
    [JNZ] = "JNZ",
    [RESIZE] = "RESIZE",
    [COPY_STRING] = "COPY_STRING",
    [IO_GEN_SLEEP] = "IO_GEN_SLEEP",
    [IO_STDIN_READ] = "IO_STDIN_READ",
    [IO_STDOUT_WRITE] = "IO_STDOUT_WRITE",
    [IO_FS_CREATE] = "IO_FS_CREATE",
    [IO_FS_DELETE] = "IO_FS_DELETE",
    [IO_FS_TRUNCATE] = "IO_FS_TRUNCATE",
    [IO_FS_WRITE] = "IO_FS_WRITE",
    [IO_FS_READ] = "IO_FS_READ",
    [EXIT] = "EXIT",
    [7] = NULL
};
char* instruccion_a_ejecutar; 
char** elementos_instrucciones;  
int cantidad_parametros;


// ------- Definiciones del ciclo ------- //
static void fetch();
static void pedir_instruccion();
static void recibir_instruccion();
static void decode();
static int buscar_comando(char *instruccion);
static void execute();
static void liberar_memoria();
static void modificar_motivo (codigo_instrucciones comando, int cantidad_parametros, char * parm1, char * parm2, char * parm3);
static void set(char* registro, char* valor); 
static void sleep_c(char* tiempo);
static void wait_c(char* recurso);
static void signal_c(char* recurso);
static void exit_c();


void ciclo_de_instruccion(){
    fetch();
    decode();
    execute();
    liberar_memoria();
}

// ------- Funciones del ciclo ------- //
static void fetch() { 
    log_info(cpu_logger,"PID: %d - FETCH - Program Counter: %d",contexto_ejecucion->pid, contexto_ejecucion->program_counter);

    //le mando el program pointer a la memoria para que me pase la instruccion a la que apunta
    pedir_instruccion();
    recibir_instruccion();
    contexto_ejecucion->program_counter +=1;
}

static void pedir_instruccion(){
    t_paquete *paquete = crear_paquete(MANDAR_INSTRUCCION);
    agregar_entero_a_paquete(paquete,contexto_ejecucion->program_counter);
    agregar_entero_a_paquete(paquete,contexto_ejecucion->pid);
    enviar_paquete(paquete, socket_cliente_memoria);
}

static void recibir_instruccion()
{
    t_paquete *paquete = recibir_paquete(socket_cliente_memoria);
    void *stream = paquete->buffer->stream;

    if (paquete->codigo_operacion == INSTRUCCION_SOLICITADA){
        instruccion_a_ejecutar = sacar_cadena_de_paquete(&stream);
    }else{
        log_error(cpu_logger,"Falla al recibir las instrucciones\n");
        abort();
    }
    eliminar_paquete(paquete);
}

static void decode(){
    elementos_instrucciones = string_n_split(instruccion_a_ejecutar, 4, " ");
    cantidad_parametros = string_array_size(elementos_instrucciones) - 1;
    instruccion_actual = buscar_comando(elementos_instrucciones[0]);
}

static int buscar_comando(char *instruccion) {
    int tamano_comandos = sizeof(comandos) / sizeof(comandos[0]);
    for (int i = 0; i < tamano_comandos; i++) {
        if (comandos[i] != NULL && strcmp(comandos[i], instruccion) == 0) {
            return i;
        }
    }
    return -1; // Comando no encontrado
}
 
static void execute() {

    switch(cantidad_parametros) {
        case 0:
            log_info(cpu_logger, "PID: %d - Ejecutando: %s", contexto_ejecucion->pid, comandos[instruccion_actual]);
            break;
        case 1:
            log_info(cpu_logger, "PID: %d - Ejecutando: %s -  %s", contexto_ejecucion->pid, comandos[instruccion_actual], elementos_instrucciones[1]);
            break;
        case 2:   
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s, %s", contexto_ejecucion->pid, comandos[instruccion_actual], elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case 3:
            log_info(cpu_logger, "PID: %d - Ejecutando: %s - %s, %s, %s", contexto_ejecucion->pid, comandos[instruccion_actual], elementos_instrucciones[1], elementos_instrucciones[2], elementos_instrucciones[3]);
            break; 
    }
    switch(instruccion_actual){
        case SET:
            set(elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case MOV_IN:
            mov_in(elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case MOV_OUT:
            mov_out(elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case SLEEP:
            sleep_c(elementos_instrucciones[1]);
            break;
        case WAIT:
            wait_c(elementos_instrucciones[1]);
            break;
        case SIGNAL:
            signal_c(elementos_instrucciones[1]);
            break;
        /*case F_OPEN:
            //f_open(elementosInstruccion[1]);
            break;
        case F_CLOSE:
            //f_close(elementosInstruccion[1]);
            break;
        case F_SEEK:
            //f_seek(elementosInstruccion[1], elementosInstruccion[2]);
            break;
        case F_READ:
            //f_read(elementosInstruccion[1], elementosInstruccion[2], elementosInstruccion[3]);
            break;
        case F_WRITE:
            //f_write(elementosInstruccion[1], elementosInstruccion[2], elementosInstruccion[3]);
            break;
        case F_TRUNCATE:
            //f_truncate(elementosInstruccion[1], elementosInstruccion[2]);
            break;*/
        case EXIT:
            exit_c();
            break;
        default:
            break;
    }
}

// ------- Funciones del SO ------- //

static void set(char* registro, char* valor){ 
    setear_registro(registro, valor);
}

static void sleep_c(char* tiempo){
    modificar_motivo (SLEEP, 1, tiempo, "", "");
    enviar_contexto(socket_cliente_dispatch);
}

static void wait_c(char* recurso){
    modificar_motivo (WAIT, 1, recurso, "", "");
    enviar_contexto(socket_cliente_dispatch);
}

static void signal_c(char* recurso){
    modificar_motivo (SIGNAL, 1, recurso, "", "");
    enviar_contexto(socket_cliente_dispatch);
}

static void exit_c() {
    char * terminado = string_duplicate ("SUCCESS");
    modificar_motivo (EXIT, 1, terminado, "", "");
    enviar_contexto(socket_cliente_dispatch);
    free (terminado);
}


// ------- Funciones auxiliares ------- //

void setear_registro(char *registros, char* valor)
{
    if (string_equals_ignore_case(registros, "AX"))
        contexto_ejecucion->AX = atoi(valor);

    if (string_equals_ignore_case(registros, "BX"))
        contexto_ejecucion->BX = atoi(valor);

    if (string_equals_ignore_case(registros, "CX"))
        contexto_ejecucion->CX = atoi(valor);

    if (string_equals_ignore_case(registros, "DX"))
        contexto_ejecucion->DX = atoi(valor);
}

static void modificar_motivo (codigo_instrucciones comando, int cantidad_parametros, char * parm1, char * parm2, char * parm3) {
    char * (parametros[3]) = { parm1, parm2, parm3 };
    contexto_ejecucion->motivo_desalojo->comando = comando;
    for (int i = 0; i < cantidad_parametros; i++)
        contexto_ejecucion->motivo_desalojo->parametros[i] = string_duplicate(parametros[i]);
    contexto_ejecucion->motivo_desalojo->cantidad_parametros = cantidad_parametros;
}

static void liberar_memoria() {
    for (int i = 0; i <= cantidad_parametros; i++) free(elementos_instrucciones[i]);
    free(elementos_instrucciones);
}



