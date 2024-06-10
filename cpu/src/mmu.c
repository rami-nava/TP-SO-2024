#include "cpu.h"

static uint32_t traducir_pagina_a_marco(uint32_t numero_pagina);
static void pedir_numero_frame(uint32_t numero_pagina);
static uint32_t recibir_numero_marco_pagina();
static char log_contenido(void* contenido_a_enviar, size_t bytes_en_este_marco);

//==================================================AUXILIARES=================================================================================


static void pedido_escritura_mmu(void* contenido, uint32_t direccion_fisica, uint32_t bytes_a_escribir){
    t_paquete* paquete = crear_paquete(ESCRIBIR_CONTENIDO_EN_MEMORIA);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, bytes_a_escribir);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    agregar_bytes_a_paquete(paquete, contenido, bytes_a_escribir);
    enviar_paquete(paquete, socket_cliente_memoria);
    free(contenido);
}

static void pedido_lectura_mmu(uint32_t direccion_fisica, uint32_t bytes_a_leer){
    t_paquete *paquete = crear_paquete(LEER_CONTENIDO_EN_MEMORIA_DESDE_CPU);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_sin_signo_a_paquete(paquete, bytes_a_leer);
    agregar_entero_sin_signo_a_paquete(paquete, direccion_fisica);
    enviar_paquete(paquete, socket_cliente_memoria);
}

static uint32_t traducir_pagina_a_marco(uint32_t numero_pagina){
    
    pedir_numero_frame(numero_pagina);
   
    uint32_t numero_marco = recibir_numero_marco_pagina();
    log_info(cpu_logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d \n", contexto_ejecucion->pid, numero_pagina, numero_marco);
   
    return numero_marco;
}

// Busca el marco en la tlb y si no lo encuentra hace un acceso a memoria 
uint32_t buscar_marco_tlb_o_memoria (uint32_t numero_pagina) {

    uint32_t numero_marco = 0;

    //Busco el marco en la tlb
    uint32_t respuesta_tlb = consultar_tlb(contexto_ejecucion->pid, numero_pagina); 
    
    if (respuesta_tlb == -1){
        
        log_info(cpu_logger, "PID: %d - TLB MISS - Página: %d \n", contexto_ejecucion->pid, numero_pagina);

        // Llamos a la  Memoria, para conseguir el número de marco correspondiente a la página
        numero_marco = traducir_pagina_a_marco(numero_pagina);

        agregar_entrada_tlb(contexto_ejecucion->pid, numero_pagina, numero_marco);

    }else if(respuesta_tlb == -2){

        //Significa que la tlb esta deshabilitada
        numero_marco = traducir_pagina_a_marco(numero_pagina);

    }else {
        log_info(cpu_logger, "PID: %d - TLB HIT - Página: %d \n", contexto_ejecucion->pid, numero_pagina);
        numero_marco = respuesta_tlb;
    }

    return numero_marco;
}

static void pedir_numero_frame(uint32_t numero_pagina){

    t_paquete *paquete = crear_paquete(TRADUCIR_PAGINA_A_MARCO);
    agregar_entero_sin_signo_a_paquete(paquete, numero_pagina);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    
    enviar_paquete(paquete, socket_cliente_memoria);
}

static uint32_t recibir_numero_marco_pagina(){
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

uint32_t traducir_de_logica_a_fisica(uint32_t direccion_logica){
    
    uint32_t numero_pagina = 0;
    uint32_t offset = 0;
    uint32_t numero_marco = 0;
    uint32_t direccion_fisica = 0;

    // Calculamos numero_pagina y offset
    numero_pagina = floor(direccion_logica / tam_pagina);
    offset = direccion_logica - (numero_pagina * tam_pagina);

    //Busco el marco de la pagina
    numero_marco = buscar_marco_tlb_o_memoria(numero_pagina); 
    
    direccion_fisica = numero_marco * tam_pagina + offset;

    return direccion_fisica;
}

uint32_t bytes(uint32_t direccion_fisica, uint32_t bytes_manipulados, uint32_t tamanio){
    uint32_t bytes = tam_pagina - (direccion_fisica % tam_pagina);
    if (bytes > tamanio - bytes_manipulados) {
    bytes = tamanio - bytes_manipulados;
    }
    return bytes;
}

//================================================== ESCRITURA=================================================================================

// Pedido de escritura generica para memoria
void escritura_en_memoria(void* contenido, uint32_t tamanio_escritura, uint32_t direccion_logica, uint32_t valor_para_log){

    uint32_t pagina_actual = floor(direccion_logica / tam_pagina);

    uint32_t marco_actual = -1;
    
    uint32_t bytes_escritos = 0;

    uint32_t direccion_fisica_actual = traducir_de_logica_a_fisica(direccion_logica);

    uint32_t bytes_en_este_marco = bytes(direccion_fisica_actual, bytes_escritos, tamanio_escritura); //bytes en este marco, pueden ser menos q el tampag si el desp no es 0

    void* contenido_a_enviar = malloc(bytes_en_este_marco); // este es el cacho de string o int que se envia en esta parte

    memcpy(contenido_a_enviar, contenido, bytes_en_este_marco); //contenido arranca en 0 pq es la primera

    pedido_escritura_mmu(contenido_a_enviar, direccion_fisica_actual, bytes_en_este_marco);
    
    uint32_t escritura_guardada;
    recv(socket_cliente_memoria, &escritura_guardada, sizeof(uint32_t), MSG_WAITALL);

    log_info(cpu_logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d \n", contexto_ejecucion->pid, direccion_fisica_actual, valor_para_log); //VER EL CASTEO DE VOID* A CHARS Y A INT
        
    bytes_escritos += bytes_en_este_marco;

    if (bytes_escritos < tamanio_escritura) {

        //Hasta aca ya copiamos la cant 'bytes_en_este_marco' del contenido total, ahora el resto se splitea
        printf("\nbytes escritos hasta el momento %d\n", bytes_escritos);
        //Se usa: *El tamanio de la escritura supera el tamanio de pagina 
        //        Y/= *El desplazamiento no es 0 y se escribe el restante en otra pagina
        
        for (int i=1; bytes_escritos < tamanio_escritura; i++){

            pagina_actual ++;

            marco_actual = buscar_marco_tlb_o_memoria(pagina_actual); 
            
            direccion_fisica_actual = marco_actual * tam_pagina;

            bytes_en_este_marco = bytes(direccion_fisica_actual, bytes_escritos, tamanio_escritura);

            contenido_a_enviar = malloc(bytes_en_este_marco);

            memcpy(contenido_a_enviar, contenido+bytes_escritos, bytes_en_este_marco); //aca se actualiza el contenido segun lo que ya se copio

            pedido_escritura_mmu(contenido_a_enviar, direccion_fisica_actual, bytes_en_este_marco);

            uint32_t escritura_guardada;
            recv(socket_cliente_memoria, &escritura_guardada, sizeof(uint32_t), MSG_WAITALL);

            log_info(cpu_logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d \n", contexto_ejecucion->pid,direccion_fisica_actual, valor_para_log); //VER EL CASTEO DE VOID* A CHARS Y A INT
            
            bytes_escritos += bytes_en_este_marco;
            printf("\nbytes escritos hasta el momento %d\n", bytes_escritos); 
        }
    }
}

//================================================== LECTURA=================================================================================
