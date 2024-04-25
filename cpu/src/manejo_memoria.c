
#include "cpu.h"

// Variables GLOBALES
uint32_t tam_pagina;

// FUNCIONES INTERNAS //
static void enviar_handshake();
static void recibir_handshake();
static uint32_t traducir_pagina_a_marco(uint32_t numero_pagina);
static void pedir_numero_frame(uint32_t numero_pagina);
static uint32_t numero_marco_pagina();
static void pedir_MOV_IN(uint32_t direccion_fisica);
static void pedir_MOV_OUT(uint32_t direccion_fisica, uint32_t registro);

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

//================================================== MMU =================================================================================

uint32_t traducir_de_logica_a_fisica(uint32_t direccion_logica){
    
    uint32_t numero_pagina = 0;
    uint32_t offset = 0;
    uint32_t numero_marco = 0;
    uint32_t direccion_fisica = 0;

    // Calculamos numero_pagina y offset
    numero_pagina = floor(direccion_logica / tam_pagina);
    offset = direccion_logica - (numero_pagina * tam_pagina);

    // Llamos a la  Memoria, para conseguir el número de marco correspondiente a la página
    numero_marco = traducir_pagina_a_marco(numero_pagina);

    // Calculamos la direcion fisica
    direccion_fisica = numero_marco * tam_pagina + offset;

    return direccion_fisica;
}

static uint32_t traducir_pagina_a_marco(uint32_t numero_pagina){
    
    pedir_numero_frame(numero_pagina);
    log_info(cpu_logger, "Pagina enviada a memoria \n");
   
    uint32_t numero_marco = numero_marco_pagina();
    log_info(cpu_logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d \n", contexto_ejecucion->pid, numero_pagina, numero_marco);
   
    return numero_marco;
}

static void pedir_numero_frame(uint32_t numero_pagina){

    t_paquete *paquete = crear_paquete(TRADUCIR_PAGINA_A_MARCO);
    agregar_entero_sin_signo_a_paquete(paquete, numero_pagina);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    
    enviar_paquete(paquete, socket_cliente_memoria);
}

static uint32_t numero_marco_pagina(){
    uint32_t numero_marco = 0;

    t_paquete *paquete = recibir_paquete(socket_cliente_memoria);
    void *stream = paquete->buffer->stream;

    if (paquete->codigo_operacion == NUMERO_MARCO){
        
        numero_marco = sacar_entero_sin_signo_de_paquete(&stream);

        eliminar_paquete(paquete);

        return numero_marco;
    }
    else{
        log_error(cpu_logger, "No me enviaste el numero de marco :( \n");
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

void mov_in(char *registro, char *direccion_logica)
{
    uint32_t direccion_fisica = traducir_de_logica_a_fisica(atoi(direccion_logica));

    if (direccion_fisica != UINT32_MAX)
    {
        pedir_MOV_IN(direccion_fisica);

        uint32_t valor_leido = 0;
        recv(socket_cliente_memoria, &valor_leido, sizeof(uint32_t), MSG_WAITALL);

        char valor_str[12]; 
        snprintf(valor_str, sizeof(valor_str), "%" PRIu32, valor_leido);

        setear_registro(registro, valor_str);

        log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %d \n", contexto_ejecucion->pid, "LEER", direccion_fisica, valor_leido);
    }
}

void mov_out(char *direccion_logica, char *registro)
{
    uint32_t valor = buscar_registro(registro);

    uint32_t direccion_fisica = traducir_de_logica_a_fisica(atoi(direccion_logica));

    if (direccion_fisica != UINT32_MAX)
    {
        pedir_MOV_OUT(direccion_fisica, valor);

        uint32_t se_ha_escrito;
        recv(socket_cliente_memoria, &se_ha_escrito, sizeof(uint32_t), MSG_WAITALL);

        log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %d \n", contexto_ejecucion->pid, "ESCRIBIR", direccion_fisica, valor);
    }
}

static void pedir_MOV_OUT(uint32_t direccion_fisica, uint32_t valor_registro)
{
    t_paquete *paquete = crear_paquete(PEDIDO_MOV_OUT);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_entero_sin_signo_a_paquete(paquete, valor_registro);
    enviar_paquete(paquete, socket_cliente_memoria);
}

static void pedir_MOV_IN(uint32_t direccion_fisica)
{
    t_paquete *paquete = crear_paquete(PEDIDO_MOV_IN);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    enviar_paquete(paquete, socket_cliente_memoria);
}