
#include "cpu.h"

// Variables GLOBALES
uint32_t tam_pagina;

// FUNCIONES INTERNAS //
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

// Leo de memoria y lo escribo en el registro
void mov_in(char *registro, char *registro_direccion_logica){

    // Buscamos la direccion logica
    uint32_t direccion_logica = buscar_registro(registro_direccion_logica);

    // Buscamos el tamanio del registro (cantidad de bytes que voy a leer)
    uint32_t tam_registro = tamanio_registro(registro);

    t_list* direcciones_fisicas_a_leer = obtener_direcciones_fisicas_mmu(tam_registro, direccion_logica);

    // Enviamos el pedido de MOV_IN
    void* valor_leido = lectura_en_memoria(tam_registro, direcciones_fisicas_a_leer);

    // lo pongo aca porque capaz para el stdout usamos la misma funcion de leer 
    // y no se si el stdout hace lo mismo que el mov in
    char ascii_char = *((char*)valor_leido);
    uint32_t valor = (uint32_t)ascii_char;

    // Actualizamos el valor del registro
    setear_registro_entero(registro, valor); 
    free(valor_leido);
}

// Escribe el valor del registro de la derecha en la direccion de la izquierda
void mov_out(char *registro_direccion_logica, char *registro){

    //devuelve un puntero al registro (lo que tengo que escribir)
    void* valor_a_escribir = buscar_valor_registro_generico(registro);

    // Devuelve la direccion logica almacenada en el registro de la izquierda
    uint32_t direccion_logica = buscar_registro(registro_direccion_logica);

    // Devuelve cantidad de bytes que se van a escribir
    uint32_t tam_registro = tamanio_registro(registro);

    // A partir de la direccion logica inicial y el tamanio total a escribir me devuelve las direcciones fisicas de las paginas que voy a necesitar
    t_list* direcciones_fisicas_a_escribir = obtener_direcciones_fisicas_mmu(tam_registro, direccion_logica);

    // Solo para el log
    uint32_t valor_registro_log = buscar_registro(registro);
    t_acceso_memoria* primera_df = ((t_acceso_memoria*)list_get(direcciones_fisicas_a_escribir, 0));

    uint32_t direccion_fisica_para_log = primera_df->direccion_fisica;
    log_info(cpu_logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d \n", contexto_ejecucion->pid, direccion_fisica_para_log, valor_registro_log); 

    escritura_en_memoria(valor_a_escribir, direcciones_fisicas_a_escribir);
}

//======================================== AUXILIARES ==============================
void pedido_escritura(void* contenido, uint32_t direccion_fisica, uint32_t bytes_a_escribir){
    t_paquete* paquete = crear_paquete(ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_CPU);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, bytes_a_escribir);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_bytes_a_paquete(paquete, contenido, bytes_a_escribir);
    enviar_paquete(paquete, socket_cliente_memoria);
    free(contenido);
}

void pedido_lectura(uint32_t direccion_fisica, uint32_t bytes_a_leer){
    t_paquete *paquete = crear_paquete(LEER_CONTENIDO_EN_MEMORIA_DESDE_CPU);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, bytes_a_leer);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    enviar_paquete(paquete, socket_cliente_memoria);
}