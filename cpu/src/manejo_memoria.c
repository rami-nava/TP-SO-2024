
#include "cpu.h"

// Variables GLOBALES
int tam_pagina;

// FUNCIONES INTERNAS //
static int traducir_pagina_a_marco(uint32_t numero_pagina);
static void pedir_numero_frame(uint32_t numero_pagina);
static int numero_marco_pagina();
static void enviar_paquete_READ(uint32_t direccion_fisica);
static uint32_t recibir_valor_a_insertar();
static void enviar_paquete_WRITE(uint32_t direccion_fisica, uint32_t registro);
static void enviar_handshake();
static void recibir_handshake();

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
        tam_pagina = sacar_entero_de_paquete(&stream);
    }
    else
    {
        log_error(cpu_logger,"No me enviaste el tam_pagina :( \n");
        abort();
    }
    eliminar_paquete(paquete);

}

//================================================== MMU =================================================================================

uint32_t traducir_de_logica_a_fisica(uint32_t direccion_logica)
{
    uint32_t numero_pagina = 0;
    uint32_t offset = 0;
    int numero_marco = 0;
    uint32_t direccion_fisica = 0;

    // Calculamos numero_pagina y offset
    numero_pagina = floor(direccion_logica / tam_pagina);
    offset = direccion_logica - (numero_pagina * tam_pagina);

    // Llamos a la  Memoria, para conseguir el número de marco correspondiente a la página
    numero_marco = traducir_pagina_a_marco(numero_pagina);

    // Le avisa a Kernel que hay Page_fault(-1)
    if (numero_marco == -1)
    {
        return UINT32_MAX;
    }

    // Calculamos la direcion fisica
    direccion_fisica = numero_marco * tam_pagina + offset;

    return direccion_fisica;
}

static int traducir_pagina_a_marco(uint32_t numero_pagina)
{
    pedir_numero_frame(numero_pagina);
    log_info(cpu_logger, "Pagina enviada a memoria \n");
    int numero_marco = numero_marco_pagina();
    log_info(cpu_logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d \n", contexto_ejecucion->pid, numero_pagina, numero_marco);
    return numero_marco;
}

static void pedir_numero_frame(uint32_t numero_pagina)
{
    t_paquete *paquete = crear_paquete(TRADUCIR_PAGINA_A_MARCO);
    agregar_entero_sin_signo_a_paquete(paquete, numero_pagina);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    enviar_paquete(paquete, socket_cliente_memoria);
}

static int numero_marco_pagina()
{
    int numero_marco = 0;

    t_paquete *paquete = recibir_paquete(socket_cliente_memoria);
    void *stream = paquete->buffer->stream;

    if (paquete->codigo_operacion == NUMERO_MARCO)
    {
        numero_marco = sacar_entero_de_paquete(&stream);

        eliminar_paquete(paquete);

        return numero_marco;
    }
    else
    {
        log_error(cpu_logger, "No me enviaste el numero de marco :( \n");
        abort();
    }
    eliminar_paquete(paquete);
}

/// FUNCIONES DE CPU Y KERNEL ///

void mov_in(char *registro, char *direccion_logica)
{

    uint32_t direccion_fisica = traducir_de_logica_a_fisica(atoi(direccion_logica));

    if (direccion_fisica != UINT32_MAX)
    {
        enviar_paquete_READ(direccion_fisica);

        int valor = recibir_valor_a_insertar(socket_cliente_memoria);

        char valor_str[20]; // Tamaño suficiente para almacenar números enteros
        snprintf(valor_str, sizeof(valor_str), "%d", valor);

        setear_registro(registro, valor_str);

        log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %d \n", contexto_ejecucion->pid, "LEER", direccion_fisica, valor);
    }
}

static void enviar_paquete_READ(uint32_t direccion_fisica)
{
    t_paquete *paquete = crear_paquete(READ);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    enviar_paquete(paquete, socket_cliente_memoria);
}

static uint32_t recibir_valor_a_insertar()
{
    uint32_t valor_registro = 0;

    t_paquete *paquete = recibir_paquete(socket_cliente_memoria);
    void *stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == VALOR_READ)
    {
        valor_registro = sacar_entero_sin_signo_de_paquete(&stream);
        return valor_registro;
    }
    else
    {
        log_error(cpu_logger, "No me enviaste el valor a leer:( \n");
        abort();
    }
    eliminar_paquete(paquete);
}

void mov_out(char *direccion_logica, char *registro)
{

    uint32_t valor = buscar_registro(registro);

    uint32_t direccion_fisica = traducir_de_logica_a_fisica(atoi(direccion_logica));

    if (direccion_fisica != UINT32_MAX)
    {
        enviar_paquete_WRITE(direccion_fisica, valor);

        int se_ha_escrito = 1;
        recv(socket_cliente_memoria, &se_ha_escrito, sizeof(int), 0);

        log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %d \n", contexto_ejecucion->pid, "ESCRIBIR", direccion_fisica, valor);
    }
}

static void enviar_paquete_WRITE(uint32_t direccion_fisica, uint32_t valor_registro)
{
    t_paquete *paquete = crear_paquete(WRITE);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_entero_sin_signo_a_paquete(paquete, valor_registro);
    enviar_paquete(paquete, socket_cliente_memoria);
}
