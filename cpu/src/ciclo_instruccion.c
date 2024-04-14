#include "cpu.h"

// Variables globales //

char *comandos[] = {
    [SET] = "SET", //2p
    [MOV_IN] = "MOV_IN", //2p
    [MOV_OUT] = "MOV_OUT", //2p
    [SUB] = "SUB", //2p
    [SUM] = "SUM", //2p
    [WAIT] = "WAIT", //1p
    [SIGNAL] = "SIGNAL", //1p
    [JNZ] = "JNZ", //2p
    [RESIZE] = "RESIZE", //1p
    [COPY_STRING] = "COPY_STRING", //1p
    [IO_GEN_SLEEP] = "IO_GEN_SLEEP", //2p
    [IO_STDIN_READ] = "IO_STDIN_READ", //3p
    [IO_STDOUT_WRITE] = "IO_STDOUT_WRITE", //3p
    [IO_FS_CREATE] = "IO_FS_CREATE", //2p
    [IO_FS_DELETE] = "IO_FS_DELETE", //2p
    [IO_FS_TRUNCATE] = "IO_FS_TRUNCATE", //3p
    [IO_FS_WRITE] = "IO_FS_WRITE", //5p
    [IO_FS_READ] = "IO_FS_READ", //5p
    [EXIT] = "EXIT", //0
    [7] = NULL
};



char* instruccion_a_ejecutar; 
char** elementos_instrucciones;  
int cantidad_parametros;
int interrupcion = 0;
bool seguir_ejecutando = true;
int tipo_interrupcion;
// ------- Definiciones del ciclo ------- //
static void fetch();
static void pedir_instruccion();
static void recibir_instruccion();
static void decode();
static int buscar_comando(char *instruccion);
static void execute();
static void check_interrupt();
static void liberar_memoria();
static void modificar_motivo (codigo_instrucciones comando, int cantidad_parametros, char * parm1, char * parm2, char * parm3);
static void set(char* registro, char* valor);
static void sum(char* registro_destino, char* registro_origen);
static void sub(char* registro_destino, char* registro_origen); 
static void jnz(char* registro, char* numero_instruccion);
static void io_gen_sleep(char* interfaz, char* unidades_trabajo);
static void io_stdin_read(char* interfaz, char* direccion_fisica, char* tamanio_registro);
static void io_stdout_write(char* interfaz, char* direccion_fisica, char* tamanio_registro);
static void wait_c(char* recurso);
static void signal_c(char* recurso);
static void exit_c();

void ciclo_de_instruccion(){
    fetch();
    decode();
    execute();
    check_interrupt();
    liberar_memoria();
}

// ------- Funciones del ciclo ------- //
static void fetch() { 
    log_info(cpu_logger,"PID: %d - FETCH - Program Counter: %d",contexto_ejecucion->pid, contexto_ejecucion->PC);

    //le mando el program pointer a la memoria para que me pase la instruccion a la que apunta
    pedir_instruccion();
    recibir_instruccion();
    contexto_ejecucion->PC +=1;
}

static void pedir_instruccion(){
    t_paquete *paquete = crear_paquete(MANDAR_INSTRUCCION);
    agregar_entero_a_paquete(paquete,contexto_ejecucion->PC);
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
    elementos_instrucciones = string_n_split(instruccion_a_ejecutar, 7, " "); 
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
    return -1; 
}
 
static void execute() {

    log_info(cpu_logger, "PID: %d - Ejecutando: %s", contexto_ejecucion->pid, instruccion_a_ejecutar);
    free(instruccion_a_ejecutar);

    switch(instruccion_actual){
        case SET:
            set(elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case SUM:
            sum(elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case SUB:
            sub(elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case JNZ:
            jnz(elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case MOV_IN:
            mov_in(elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case MOV_OUT:
            mov_out(elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case WAIT:
            wait_c(elementos_instrucciones[1]);
            break;
        case SIGNAL:
            signal_c(elementos_instrucciones[1]);
            break;
        case RESIZE:
            //resize(elementosInstruccion[1]);
            break;
        case COPY_STRING:
            //copy_string(elementosInstruccion[1]);
            break;
        case IO_GEN_SLEEP:
            io_gen_sleep(elementos_instrucciones[1], elementos_instrucciones[2]);
            break;
        case IO_STDIN_READ:
            io_stdin_read(elementos_instrucciones[1], elementos_instrucciones[2], elementos_instrucciones[3]);
            break;
        case IO_STDOUT_WRITE:
            io_stdout_write(elementos_instrucciones[1], elementos_instrucciones[2], elementos_instrucciones[3]);
            break;
        case IO_FS_CREATE:
            //io_fs_create(elementosInstruccion[1], elementosInstruccion[2]);
            break;
        case IO_FS_DELETE:
            //io_fs_delete(elementosInstruccion[1], elementosInstruccion[2]);
            break;
        case IO_FS_TRUNCATE:
            //io_fs_truncate(elementosInstruccion[1], elementosInstruccion[2], elementosInstruccion[3]);
            break;
        case IO_FS_WRITE:
            //io_fs_truncate(elementosInstruccion[1], elementosInstruccion[2], elementosInstruccion[3], elementosInstruccion[4], elementosInstruccion[5]);
            break;
        case IO_FS_READ:
            //io_fs_truncate(elementosInstruccion[1], elementosInstruccion[2], elementosInstruccion[3], elementosInstruccion[4], elementosInstruccion[5]);
            break;
        case EXIT:
            exit_c();
            break;
        default:
            break;
    }
}

static void check_interrupt(){

    pthread_mutex_lock(&interrupcion_mutex);
    if(interrupcion != 0){
        interrupcion = 0;
        pthread_mutex_unlock(&interrupcion_mutex);
        pthread_mutex_lock(&seguir_ejecutando_mutex);
        seguir_ejecutando = false;
        pthread_mutex_unlock(&seguir_ejecutando_mutex);

        pthread_mutex_lock(&interrupcion_mutex);

        if(tipo_interrupcion == 1){
            log_info(cpu_logger, "Recibi una interrupcion de Fin de Quantum\n");
            modificar_motivo (FIN_QUANTUM, 0, "", "", "");
        }
        else if(tipo_interrupcion == 2){
            log_info(cpu_logger, "Recibi una interrupcion de Error de IO\n");
            modificar_motivo (EXIT, 1, "Error IO", "", "");
        }
        else{
            perror("Recibi un codigo de interrupcion desconocido");
        }

        enviar_contexto(socket_cliente_dispatch);
    }
    pthread_mutex_unlock(&interrupcion_mutex);
}

void atender_interrupt(void * socket_servidor_interrupt){
    int conexion = *(int*)socket_servidor_interrupt;

    while(1){
        t_paquete *paquete = recibir_paquete(conexion);
        void* stream = paquete->buffer->stream;

        if (paquete->codigo_operacion == DESALOJO){
            tipo_interrupcion = sacar_entero_de_paquete(&stream);
            
            pthread_mutex_lock(&interrupcion_mutex);
            interrupcion ++;
            pthread_mutex_unlock(&interrupcion_mutex);
        }
        else{
            log_error(cpu_logger,"No recibi un codigo de interrupcion");
            abort();
        }
        eliminar_paquete(paquete);
    }

}
// ------- Funciones del SO ------- //
static void set(char* registro, char* valor){ 
    setear_registro(registro, valor);
}

static void sum(char* registro_destino, char* registro_origen){ 
    int valor1 = buscar_registro(registro_destino);
    int valor2 = buscar_registro(registro_origen);  

    int suma = valor1 + valor2;
    
    //convierte la suma a una cadena de caracteres antes de pasarlo a setear_registro
    char suma_str[20]; // Tamaño suficiente para almacenar números enteros
    snprintf(suma_str, sizeof(suma_str), "%d", suma);

    setear_registro(registro_destino, suma_str);
}

static void sub(char* registro_destino, char* registro_origen){ 
    int valor1 = buscar_registro(registro_destino);
    int valor2 = buscar_registro(registro_origen);

    int resta = valor1 - valor2;
    
    //convierte la resta a una cadena de caracteres antes de pasarlo a setear_registro
    char resta_str[20]; // Tamaño suficiente para almacenar números enteros
    snprintf(resta_str, sizeof(resta_str), "%d", resta);

    setear_registro(registro_destino, resta_str);
}


static void jnz(char* registro, char* numero_instruccion){
    if (buscar_registro(registro) != 0){
        contexto_ejecucion->PC = atoi(numero_instruccion); 
    }
}

static void io_gen_sleep(char* interfaz, char* unidades_trabajo){
    modificar_motivo(IO_GEN_SLEEP, 2, interfaz, unidades_trabajo, "");
    enviar_contexto(socket_cliente_dispatch);
}

static void io_stdin_read(char* interfaz, char* direccion_fisica, char* tamanio_registro){
    modificar_motivo(IO_STDIN_READ, 2, interfaz, direccion_fisica, tamanio_registro);
    enviar_contexto(socket_cliente_dispatch);
}

static void io_stdout_write(char* interfaz, char* direccion_fisica, char* tamanio_registro){
    modificar_motivo(IO_STDOUT_WRITE, 2, interfaz, direccion_fisica, tamanio_registro);
    enviar_contexto(socket_cliente_dispatch);
}

static void wait_c(char* recurso){
    modificar_motivo (WAIT, 1, recurso, "", "");
    enviar_contexto(socket_cliente_dispatch);

    pthread_mutex_lock(&seguir_ejecutando_mutex);
    seguir_ejecutando = false;
    pthread_mutex_unlock(&seguir_ejecutando_mutex);
}

static void signal_c(char* recurso){
    modificar_motivo (SIGNAL, 1, recurso, "", "");
    enviar_contexto(socket_cliente_dispatch);

    pthread_mutex_lock(&seguir_ejecutando_mutex);
    seguir_ejecutando = false;
    pthread_mutex_unlock(&seguir_ejecutando_mutex);
}

static void exit_c() {
    modificar_motivo (EXIT, 1, "SUCCESS", "", "");
    enviar_contexto(socket_cliente_dispatch);
}


// ------- Funciones auxiliares registros ------- //

void setear_registro(char *registros, char* valor)
{
    if (string_equals_ignore_case(registros, "PC"))
        contexto_ejecucion->PC = atoi(valor);
    if (string_equals_ignore_case(registros, "AX"))
        contexto_ejecucion->AX = atoi(valor);
    if (string_equals_ignore_case(registros, "BX"))
        contexto_ejecucion->BX = atoi(valor);
    if (string_equals_ignore_case(registros, "CX"))
        contexto_ejecucion->CX = atoi(valor);
    if (string_equals_ignore_case(registros, "DX"))
        contexto_ejecucion->DX = atoi(valor);
    if (string_equals_ignore_case(registros, "EAX"))
        contexto_ejecucion->EAX = atoi(valor);
    if (string_equals_ignore_case(registros, "EBX"))
        contexto_ejecucion->EBX = atoi(valor);
    if (string_equals_ignore_case(registros, "ECX"))
        contexto_ejecucion->ECX = atoi(valor);
    if (string_equals_ignore_case(registros, "EDX"))
        contexto_ejecucion->EDX = atoi(valor);
    if (string_equals_ignore_case(registros, "SI"))
        contexto_ejecucion->SI = atoi(valor);
    if (string_equals_ignore_case(registros, "DI"))
        contexto_ejecucion->DI = atoi(valor);

    log_info(cpu_logger, "PID %d - Registro: %s - Valor: %s", contexto_ejecucion->pid, registros, valor);
}

int buscar_registro(char *registro)
{
    int valor = 0;

    if (string_equals_ignore_case(registro, "PC"))
        valor = (int) contexto_ejecucion->PC;

    if (string_equals_ignore_case(registro, "AX"))
        valor = (int) contexto_ejecucion->AX;

    if (string_equals_ignore_case(registro, "BX"))
        valor = (int) contexto_ejecucion->BX;

    if (string_equals_ignore_case(registro, "CX"))
        valor = (int) contexto_ejecucion->CX;

    if (string_equals_ignore_case(registro, "DX"))
        valor = (int) contexto_ejecucion->DX;

    if (string_equals_ignore_case(registro, "EAX"))
        valor = (int) contexto_ejecucion->EAX;

    if (string_equals_ignore_case(registro, "EBX"))
        valor = (int) contexto_ejecucion->EBX;

    if (string_equals_ignore_case(registro, "ECX"))
        valor = (int) contexto_ejecucion->ECX;

    if (string_equals_ignore_case(registro, "EDX"))
        valor = (int) contexto_ejecucion->EDX;

    if (string_equals_ignore_case(registro, "SI"))
        valor = (int) contexto_ejecucion->SI;

    if (string_equals_ignore_case(registro, "DI"))
        valor = (int) contexto_ejecucion->DI;

    return valor;
}

static void modificar_motivo (codigo_instrucciones comando, int cantidad_parametros, char * parm1, char * parm2, char * parm3) { // agregar mas params para las nuevas instrucciones
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




