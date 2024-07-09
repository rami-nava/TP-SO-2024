#include "cpu.h"

static uint32_t traducir_pagina_a_marco(uint32_t numero_pagina);
static void pedir_numero_frame(uint32_t numero_pagina);
static uint32_t recibir_numero_marco_pagina();
static uint32_t bytes_en_marco(uint32_t direccion_fisica, uint32_t bytes_manipulados, uint32_t tamanio);

pthread_mutex_t mutex_direcciones_fisicas;

//==================================================AUXILIARES=================================================================================

static uint32_t traducir_pagina_a_marco(uint32_t numero_pagina){
    
    pedir_numero_frame(numero_pagina);
   
    uint32_t numero_marco = recibir_numero_marco_pagina();
    
    log_info(cpu_logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d \n", contexto_ejecucion->pid, numero_pagina, numero_marco);
   
    return numero_marco;
}

// Busca el marco en la tlb y si no lo encuentra hace un acceso a memoria 
uint32_t buscar_marco_tlb_o_memoria(uint32_t numero_pagina) {

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
    numero_pagina = floor((double)direccion_logica / (double)tam_pagina);
    offset = direccion_logica - (numero_pagina * tam_pagina);

    //Busco el marco de la pagina
    numero_marco = buscar_marco_tlb_o_memoria(numero_pagina); 
    
    direccion_fisica = numero_marco * tam_pagina + offset;

    return direccion_fisica;
}

//ME DA TODAS LAS DFS QUE NECESITO Y CUANTO ESCRIBIR EN CADA UNA PARA UNA ACCION (ESCRITURA-LECTURA) EN MEMORIA
t_list* obtener_direcciones_fisicas_mmu(uint32_t tamanio_total, uint32_t direccion_logica_inicial){

    uint32_t marco_actual = -1;
    uint32_t bytes_cargados = 0;
    double cantidad_marcos = ceil((double)tamanio_total / (double)tam_pagina);

    uint32_t cantidad_de_marcos_total = (uint32_t)cantidad_marcos;

    t_list* lista_direcciones_fisicas = list_create();

    //PRIMERA PAGINA/MARCO
    uint32_t pagina_actual = floor((double)direccion_logica_inicial / (double)tam_pagina);
    uint32_t direccion_fisica_actual = traducir_de_logica_a_fisica(direccion_logica_inicial);
    uint32_t bytes_marco_actual = bytes_en_marco(direccion_fisica_actual, bytes_cargados, tamanio_total);

    // Guardamos la direccion fisica y tamanio del primer marco en la lista
    t_acceso_memoria* acceso = malloc(sizeof(t_acceso_memoria));
    acceso->direccion_fisica = direccion_fisica_actual;
    acceso->tamanio = bytes_marco_actual;

    list_add(lista_direcciones_fisicas, acceso);

    // Actualizamos el contador de bytes cargados
    bytes_cargados += bytes_marco_actual;

    //SI EL TAMAÑO ES MAYOR A LA PAGINA, SE PARTE EL DATO ENTRE MARCOS
    if (bytes_cargados < tamanio_total){

        for (int i = 1; i < cantidad_de_marcos_total || bytes_cargados < tamanio_total; i++){
            
            //SIGUENTE PAGINA/MARCO
            pagina_actual ++;
            marco_actual = buscar_marco_tlb_o_memoria(pagina_actual); 
            
            //Las dfs siguientes al primer marco comenzaran todas en desplazamiento=0 porque se busca completar el dato entero
            direccion_fisica_actual = marco_actual * tam_pagina; 
            bytes_marco_actual = bytes_en_marco(direccion_fisica_actual, bytes_cargados, tamanio_total);

            t_acceso_memoria* acceso_nuevo = malloc(sizeof(t_acceso_memoria));
            acceso_nuevo->direccion_fisica = direccion_fisica_actual;
            acceso_nuevo->tamanio = bytes_marco_actual;

            list_add(lista_direcciones_fisicas, acceso_nuevo);

            bytes_cargados += bytes_marco_actual;
        }
    }
    
    return lista_direcciones_fisicas;
}

static uint32_t bytes_en_marco(uint32_t direccion_fisica, uint32_t bytes_manipulados, uint32_t tamanio){
    uint32_t bytes = tam_pagina - (direccion_fisica % tam_pagina);
    if (bytes > tamanio - bytes_manipulados) {
    bytes = tamanio - bytes_manipulados;
    }
    return bytes;
}
