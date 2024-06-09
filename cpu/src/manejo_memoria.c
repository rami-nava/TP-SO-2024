
#include "cpu.h"

// Variables GLOBALES
uint32_t tam_pagina;

// FUNCIONES INTERNAS //
static void enviar_handshake();
static void recibir_handshake();
static uint32_t traducir_pagina_a_marco(uint32_t numero_pagina);
static void pedir_numero_frame(uint32_t numero_pagina);
static uint32_t numero_marco_pagina();
static void pedir_MOV_IN(uint32_t direccion_fisica, uint32_t tamanio);
static void pedir_MOV_OUT(uint32_t direccion_fisica, void* valor_registro, uint32_t tamanio_registro);
static uint32_t recibir_resultado_mov_in(uint32_t tam_registro);


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

        modificar_motivo(OUT_OF_MEMORY, 1, "Out of memory", "", "", "", "");
        enviar_contexto(socket_cliente_dispatch);
    }
}

void copy_string(char* tamanio)
{
    uint32_t cantidad_bytes_a_copiar = atoi(tamanio);
    uint32_t posicion_de_string_a_copiar = buscar_registro("SI");
    uint32_t posicion_de_memoria_donde_copiar = buscar_registro("DI");

    uint32_t direccion_fisica_a_copiar = traducir_de_logica_a_fisica(posicion_de_string_a_copiar);
    uint32_t direccion_fisica_donde_copiar = traducir_de_logica_a_fisica(posicion_de_memoria_donde_copiar);
    
    t_paquete *paquete = crear_paquete(PEDIDO_COPY_STRING);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, cantidad_bytes_a_copiar);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica_a_copiar);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica_donde_copiar);
    enviar_paquete(paquete, socket_cliente_memoria);
}

// leo de memoria y lo escribo en el registro
void mov_in(char *registro, char *registro_direccion_logica){

    // Buscamos la direccion logica
    uint32_t direccion_logica = buscar_registro(registro_direccion_logica);

    // Traducimos la direccion logica a fisica
    uint32_t direccion_fisica = traducir_de_logica_a_fisica(direccion_logica);

    // Buscamos el tamanio del registro
    uint32_t tam_registro = tamanio_registro(registro);

    // Enviamos el pedido de MOV_IN
    pedir_MOV_IN(direccion_fisica, tam_registro);

    // Recibimos el valor del MOV_IN
    uint32_t valor_leido = recibir_resultado_mov_in(tam_registro);

    // Actualizamos el valor del registro
    setear_registro_entero(registro, valor_leido); 

    log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %d \n", contexto_ejecucion->pid, "LEER", direccion_fisica, valor_leido);
}

static uint32_t recibir_resultado_mov_in(uint32_t tam_registro){
    int cod_op = recibir_operacion(socket_cliente_memoria);

    if (cod_op == RESULTADO_MOV_IN){

        void* buffer_dato = recibir_buffer(socket_cliente_memoria);

        uint32_t dato_a_guardar;
    
        memcpy(&dato_a_guardar, buffer_dato, sizeof(uint32_t));

        free(buffer_dato);

        return dato_a_guardar;
    }
    else{
        log_error(cpu_logger, "No me enviaste el valor :( \n");
        abort();
    }
}

void mov_out(char *registro_direccion_logica, char *registro){

    //devuelve un puntero al registro
    void* valor_registro = buscar_valor_registro_generico(registro);

    //solo para mostrarlo en el log
    uint32_t valor_registro_log = buscar_registro(registro);

    uint32_t direccion_logica = buscar_registro(registro_direccion_logica);

    uint32_t direccion_fisica = traducir_de_logica_a_fisica(direccion_logica);

    uint32_t tam_registro = tamanio_registro(registro);

    pedir_MOV_OUT(direccion_fisica, valor_registro, tam_registro);

    uint32_t escritura_guardada;
    recv(socket_cliente_memoria, &escritura_guardada, sizeof(uint32_t), MSG_WAITALL);

    log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %d \n", contexto_ejecucion->pid, "ESCRIBIR", direccion_fisica, valor_registro_log);
}


static void pedir_MOV_OUT(uint32_t direccion_fisica, void* valor_registro, uint32_t tamanio_registro){
    
    t_paquete *paquete = crear_paquete(PEDIDO_MOV_OUT);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio_registro);
    agregar_bytes_a_paquete(paquete, valor_registro, tamanio_registro);
    enviar_paquete(paquete, socket_cliente_memoria);
}

static void pedir_MOV_IN(uint32_t direccion_fisica, uint32_t tamanio){

    t_paquete *paquete = crear_paquete(PEDIDO_MOV_IN);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_entero_sin_signo_a_paquete(paquete, tamanio);
    enviar_paquete(paquete, socket_cliente_memoria);
}

