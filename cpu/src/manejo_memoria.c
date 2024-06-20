
#include "cpu.h"

// Variables GLOBALES
uint32_t tam_pagina;

// FUNCIONES INTERNAS //
static void enviar_handshake();
static void recibir_handshake();
static void* recibir_resultado_lectura();
static void loggear_escritura_en_memoria(char* registro, t_list* direcciones_fisicas);
static void loggear_lectura_en_memoria(void* contenido_leido_total, t_list* lista_accesos_memoria);
static uint32_t casteo_de_void_a_uint32_t(void* void_a_convertir);
static void pedido_escritura(void* contenido, uint32_t direccion_fisica, uint32_t bytes_a_escribir);
static void pedido_lectura(uint32_t direccion_fisica, uint32_t bytes_a_leer);
//================================================== Handshake =====================================================================
void realizar_handshake()
{
    enviar_handshake();
    recibir_handshake();
}

static void enviar_handshake()
{
    t_paquete *paquete = crear_paquete(HANDSHAKE);
    agregar_entero_a_paquete(paquete, 1);
    enviar_paquete(paquete, socket_cliente_memoria);
}

static void recibir_handshake()
{
    t_paquete* paquete = recibir_paquete(socket_cliente_memoria);
    void* stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == HANDSHAKE)
    {
        tam_pagina = sacar_entero_sin_signo_de_paquete(&stream);
    }
    else
    {
        log_error(cpu_logger,"No me enviaste el tam_pagina :( \n");
        abort();
    }
    eliminar_paquete(paquete);

}

//================================================== INSTRUCCIONES =================================================================================

// RESIZE - Aumentar o disminuir el tamanio del proceso en memoria
void resize(char *tamanio)
{
    int tamanio_proceso = atoi(tamanio);
    int out_of_memory = -1;
    
    t_paquete *paquete = crear_paquete(PEDIDO_RESIZE);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_a_paquete(paquete, tamanio_proceso);
    enviar_paquete(paquete, socket_cliente_memoria);

    recv(socket_cliente_memoria, &out_of_memory, sizeof(int), MSG_WAITALL);
    
    //En el caso que se reciba un 1, significa que no hay memoria suficiente
    if(out_of_memory == 1){
        
        pthread_mutex_lock(&seguir_ejecutando_mutex);
        seguir_ejecutando = false;
        pthread_mutex_unlock(&seguir_ejecutando_mutex);

        modificar_motivo(OUT_OF_MEMORY, 1, "OUT_OF_MEMORY", "", "", "");
        enviar_contexto(socket_cliente_dispatch);
    }
}

// COPY STRING - REVISAR
void copy_string(char* registro_tamanio)
{
    uint32_t cantidad_bytes_a_copiar = buscar_registro(registro_tamanio);
    uint32_t posicion_de_string_a_copiar = buscar_registro("SI");
    uint32_t posicion_de_memoria_donde_copiar = buscar_registro("DI");

    // Puede ser que lo tengo para leer este en paginas distintas
    t_list* direcciones_fisicas_donde_leer = obtener_direcciones_fisicas_mmu(cantidad_bytes_a_copiar, posicion_de_string_a_copiar);
    t_list* direcciones_fisicas_donde_escribir = obtener_direcciones_fisicas_mmu(cantidad_bytes_a_copiar, posicion_de_memoria_donde_copiar);

    // Leo de memoria a apartir de esta direccion fisica la cantidad_bytes_a_copiar
    void* string_a_copiar = lectura_en_memoria(cantidad_bytes_a_copiar, direcciones_fisicas_donde_leer);

    // Lo escribo en memoria a la posicion donde queremos copiarlo
    escritura_en_memoria(string_a_copiar, direcciones_fisicas_donde_escribir);
}

// MOV_IN - Lee un valor de memoria, a partir de una direccion, y lo almacena en un registro
void mov_in(char *registro, char *registro_direccion_logica){

    uint32_t direccion_logica = buscar_registro(registro_direccion_logica);

    uint32_t cantidad_bytes_a_leer = tamanio_registro(registro);

    t_list* direcciones_fisicas_a_leer = obtener_direcciones_fisicas_mmu(cantidad_bytes_a_leer, direccion_logica);

    void* valor_leido = lectura_en_memoria(cantidad_bytes_a_leer, direcciones_fisicas_a_leer);

    uint32_t valor_nuevo_registro = casteo_de_void_a_uint32_t(valor_leido);

    // Actualizamos el valor del registro
    setear_registro_entero(registro, valor_nuevo_registro); 
    free(valor_leido);
}

// MOV_OUT - Lee un valor de un registro y lo escribe en una direccion fisica
void mov_out(char *registro_direccion_logica, char *registro){

    // Puntero al valor a escribir del registro
    void* valor_a_escribir = buscar_valor_registro_generico(registro);

    uint32_t direccion_logica = buscar_registro(registro_direccion_logica);

    uint32_t cantidad_bytes_a_escribir = tamanio_registro(registro);

    t_list* direcciones_fisicas_a_escribir = obtener_direcciones_fisicas_mmu(cantidad_bytes_a_escribir, direccion_logica);

    loggear_escritura_en_memoria(registro, direcciones_fisicas_a_escribir);

    escritura_en_memoria(valor_a_escribir, direcciones_fisicas_a_escribir);
}

//================================================== ESCRITURA =================================================================================
void escritura_en_memoria(void* contenido, t_list* lista_accesos_memoria){

    uint32_t escritura_ok;
    uint32_t tamanio_copiado_actual = 0;

    t_list_iterator* iterator = list_iterator_create(lista_accesos_memoria);

    // Mientras haya una df a escribir, sigo escribiendo
    while (list_iterator_has_next(iterator)) {

        // Obtengo el próximo acceso de la lista
        t_acceso_memoria* acceso = (t_acceso_memoria*) list_iterator_next(iterator);
        
        // Buffer donde se va a copiar el contenido de a poco
        void* contenido_a_escribir_actual = malloc(acceso->tamanio);

        // Copio el contenido en el buffer
        memcpy(contenido_a_escribir_actual, contenido + tamanio_copiado_actual, acceso->tamanio);

        // Lo escribo en memoria
        pedido_escritura(contenido_a_escribir_actual, acceso->direccion_fisica, acceso->tamanio);
        recv(socket_cliente_memoria, &escritura_ok, sizeof(uint32_t), MSG_WAITALL);

        if (escritura_ok == 1){
            tamanio_copiado_actual += acceso->tamanio; 
        }else{
            log_error(cpu_logger, "Escritura fallida\n");
            abort();
        }        
    }

    list_iterator_destroy(iterator);
    
    //si ejecuta copystring la funcion de loggear escritura en memoria no sirve y el list_destroy al final
    //hace que no pueda hacer este log en la funcion de copystring
    if(instruccion_actual == COPY_STRING){
        uint32_t valor_a_escribir_para_log = casteo_de_void_a_uint32_t(contenido);
        t_acceso_memoria* primera_df =  list_get(lista_accesos_memoria, 0); 

        log_info(cpu_logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d \n", contexto_ejecucion->pid, primera_df->direccion_fisica , valor_a_escribir_para_log); 
    }

    list_destroy_and_destroy_elements(lista_accesos_memoria, free);
}

//================================================== LECTURA =================================================================================

void* lectura_en_memoria(uint32_t tamanio_total, t_list* lista_accesos_memoria){

    uint32_t tamanio_leido_actual = 0;

    // Buffer donde se va a ir almacenando el contenido leído final
    void* contenido_leido_total = malloc(tamanio_total); 

    t_list_iterator* iterator = list_iterator_create(lista_accesos_memoria);

     // Mientras haya una df a escribir, sigo escribiendo
    while (list_iterator_has_next(iterator)) {

        // Obtengo el próximo acceso de la lista
        t_acceso_memoria* acceso = (t_acceso_memoria*) list_iterator_next(iterator);
        
        // Leo en Memoria
        pedido_lectura(acceso->direccion_fisica, acceso->tamanio);

        // Guardo en un buffer la lectura y lo copio en el contenido total
        void* valor_leido = recibir_resultado_lectura(); 
        memcpy(contenido_leido_total + tamanio_leido_actual, valor_leido, acceso->tamanio);

        // Actualizo el tamanio leido
        tamanio_leido_actual += acceso->tamanio;

        // Libero la memoria del buffer
        free(valor_leido); 
    }

    loggear_lectura_en_memoria(contenido_leido_total, lista_accesos_memoria);

    list_iterator_destroy(iterator);
    
    list_destroy_and_destroy_elements(lista_accesos_memoria, free);

    return contenido_leido_total;
}

//======================================== AUXILIARES ==============================

static void pedido_escritura(void* contenido, uint32_t direccion_fisica, uint32_t bytes_a_escribir){
    t_paquete* paquete = crear_paquete(ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_CPU);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, bytes_a_escribir);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_bytes_a_paquete(paquete, contenido, bytes_a_escribir);
    enviar_paquete(paquete, socket_cliente_memoria);
    free(contenido);
}

static void pedido_lectura(uint32_t direccion_fisica, uint32_t bytes_a_leer){
    t_paquete *paquete = crear_paquete(LEER_CONTENIDO_EN_MEMORIA_DESDE_CPU);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, bytes_a_leer);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    enviar_paquete(paquete, socket_cliente_memoria);
}

static void* recibir_resultado_lectura(){
    t_paquete *paquete = recibir_paquete(socket_cliente_memoria);
    void *stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == RESULTADO_MOV_IN){
        
        void* dato_a_guardar = sacar_bytes_de_paquete(&stream); //Tendria que usar el tamanio?? TODO

        eliminar_paquete(paquete);

        return dato_a_guardar;
    }
    else{
        log_error(cpu_logger, "No me enviaste el resultado :( \n");
        abort();
    }
    eliminar_paquete(paquete);
}

static void loggear_escritura_en_memoria(char* registro, t_list* direcciones_fisicas)
{
    uint32_t valor_registro_log = buscar_registro(registro);
    t_acceso_memoria* primera_df = ((t_acceso_memoria*)list_get(direcciones_fisicas, 0));
 
    log_info(cpu_logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d \n", contexto_ejecucion->pid, primera_df->direccion_fisica, valor_registro_log); 
}

static void loggear_lectura_en_memoria(void* contenido_leido_total, t_list* lista_accesos_memoria)
{
    uint32_t valor = casteo_de_void_a_uint32_t(contenido_leido_total);
    t_acceso_memoria* primera_df = ((t_acceso_memoria*)list_get(lista_accesos_memoria, 0));

    log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %u \n", contexto_ejecucion->pid, "LEER", primera_df->direccion_fisica, valor);
}

static uint32_t casteo_de_void_a_uint32_t(void* void_a_convertir)
{
    char valor_como_char = *((char*)void_a_convertir);
    uint32_t valor = (uint32_t)valor_como_char;

    return valor;
}