#include "cpu.h"

static uint32_t traducir_pagina_a_marco(uint32_t numero_pagina);
static void pedir_numero_frame(uint32_t numero_pagina);
static uint32_t recibir_numero_marco_pagina();
static void* recibir_resultado_mov_in();

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
    numero_pagina = floor(direccion_logica / tam_pagina);
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

    t_list* lista_direcciones_fisicas = list_create();

    //PRIMERA PAGINA/MARCO
    uint32_t pagina_actual = floor(direccion_logica_inicial / tam_pagina);
    uint32_t direccion_fisica_actual = traducir_de_logica_a_fisica(direccion_logica_inicial);
    uint32_t bytes_en_este_marco = bytes(direccion_fisica_actual, bytes_cargados, tamanio_total);

    t_acceso_memoria* acceso = malloc(sizeof(t_acceso_memoria));
    acceso->direccion_fisica = direccion_fisica_actual;
    acceso->tamanio = bytes_en_este_marco;

    list_add(lista_direcciones_fisicas, acceso);

    bytes_cargados += bytes_en_este_marco;

    // PAGINAS/MARCOS SIGUIENTES SI ES QUE HAY

    if (bytes_cargados < tamanio_total){
        //EL DATO DEBERA PARTIRSE, PORQUE TAMAÑO ES MAYOR A LA PAGINA, O HAY DESPLAZAMIENTO, ETC.

        for (int i=1; bytes_cargados < tamanio_total; i++){

            pagina_actual ++;
            marco_actual = buscar_marco_tlb_o_memoria(pagina_actual); 
            
            //Las dfs siguientes al primer marco comenzaran todas en desplazamiento=0 porque se busca completar el dato entero
            direccion_fisica_actual = marco_actual * tam_pagina; 
            bytes_en_este_marco = bytes(direccion_fisica_actual, bytes_cargados, tamanio_total);

            t_acceso_memoria* acceso_nuevo = malloc(sizeof(t_acceso_memoria));
            acceso_nuevo->direccion_fisica = direccion_fisica_actual;
            acceso_nuevo->tamanio = bytes_en_este_marco;

            list_add(lista_direcciones_fisicas, acceso_nuevo);

            bytes_cargados += bytes_en_este_marco;
        }
    }
    return lista_direcciones_fisicas;
}


//mmu traduce una lista de direccions
// uso esa en las de memoria
// en io las mando al kernel y este se las manda a las io


//================================================== ESCRITURA =================================================================================

uint32_t bytes(uint32_t direccion_fisica, uint32_t bytes_manipulados, uint32_t tamanio){
    uint32_t bytes = tam_pagina - (direccion_fisica % tam_pagina);
    if (bytes > tamanio - bytes_manipulados) {
    bytes = tamanio - bytes_manipulados;
    }
    return bytes;
}

void escritura_en_memoria(void* contenido, t_list* lista_accesos_memoria){

    //RECORRO TODAS LAS DFS QUE HAYA EN LA LISTA QUE ME HIZO PREVIAMENTE LA MMU Y COPIO EN MEMORIA
    uint32_t escritura_ok;
    uint32_t tamanio_copiado_actual = 0;

    t_list_iterator* iterator = list_iterator_create(lista_accesos_memoria);
    while (list_iterator_has_next(iterator)) {

        t_acceso_memoria* acceso = (t_acceso_memoria*) list_iterator_next(iterator);
        
        void* contenido_a_escribir_actual = malloc(acceso->tamanio);

        memcpy(contenido_a_escribir_actual, contenido + tamanio_copiado_actual, acceso->tamanio);

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
    
    //CUANDO SE TERMINA DE HACER LA ESCRITURA, HAY QUE MATAR LA LISTA DE DFS SINO QUEDA MEMORIA VOLANDO EN CUALQUIER LADO
    //TODO : HACER LO MISMO CUANDO SE HAGA ESCRITURA/LECTURA EN IO
    list_destroy(lista_accesos_memoria);
}

// En la lectura me dan el tamanio de lo que leo
void* lectura_en_memoria(uint32_t tamanio_total, t_list* lista_accesos_memoria){

    uint32_t tamanio_leido_actual = 0;
    void* contenido_leido_total = malloc(tamanio_total); 

    t_acceso_memoria* primera_df = ((t_acceso_memoria*)list_get(lista_accesos_memoria, 0));
    uint32_t direccion_fisica_para_log = primera_df->direccion_fisica;

    t_list_iterator* iterator = list_iterator_create(lista_accesos_memoria);
    while (list_iterator_has_next(iterator)) {

        t_acceso_memoria* acceso = (t_acceso_memoria*) list_iterator_next(iterator);
        
        pedido_lectura(acceso->direccion_fisica, acceso->tamanio);

        void* valor_leido = malloc(acceso->tamanio);
        valor_leido = recibir_resultado_mov_in(); //si todos reciben la misma instruccion rompe
        memcpy(contenido_leido_total + tamanio_leido_actual, valor_leido, acceso->tamanio);

        tamanio_leido_actual += acceso->tamanio;

        free(valor_leido);        
        free(acceso);
        
    }

    char valor_como_char = *((char*)contenido_leido_total);
    uint32_t valor = (uint32_t)valor_como_char;

    log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %u \n", contexto_ejecucion->pid, "LEER", direccion_fisica_para_log, valor);

    list_iterator_destroy(iterator);
    
    //CUANDO SE TERMINA DE HACER LA LECTURA, HAY QUE MATAR LA LISTA DE DFS SINO QUEDA MEMORIA VOLANDO EN CUALQUIER LADO
    list_destroy(lista_accesos_memoria);

    return contenido_leido_total;
}

/* Pedido de escritura generica para memoria
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
        }
    }
}

// Pedido de lectura generica para memoria
void* lectura_en_memoria(uint32_t tamanio_lectura, uint32_t direccion_logica){

    uint32_t pagina_actual = floor(direccion_logica / tam_pagina);

    uint32_t marco_actual = -1;
    
    uint32_t bytes_leidos = 0;

    // Lo que se va a escribir en el registro
    void* contenido_leido_total = malloc(tamanio_lectura); 

    uint32_t direccion_fisica_actual = traducir_de_logica_a_fisica(direccion_logica);
    uint32_t direccion_fisica_pedida_para_log = direccion_fisica_actual;

    uint32_t bytes_en_este_marco = bytes(direccion_fisica_actual, bytes_leidos, tamanio_lectura); //bytes en este marco, pueden ser menos q el tampag si el desp no es 0

    pedido_lectura_mmu(direccion_fisica_actual, bytes_en_este_marco);

    // Buffer para cada dato que recibimos
    void* valor_leido_en_este_marco = malloc(bytes_en_este_marco);
    valor_leido_en_este_marco = recibir_resultado_mov_in();
    memcpy(contenido_leido_total, valor_leido_en_este_marco, bytes_en_este_marco);
    free(valor_leido_en_este_marco);
        
    bytes_leidos += bytes_en_este_marco;

    if (bytes_leidos < tamanio_lectura) {

        //Hasta aca ya leimos la cant 'bytes_en_este_marco' del contenido total, ahora el resto se splitea
        //Se usa: *El tamanio de la escritura supera el tamanio de pagina 
        //        Y/= *El desplazamiento no es 0 y se escribe el restante en otra pagina
        
        for (int i=1; bytes_leidos < tamanio_lectura; i++){

            pagina_actual ++;

            marco_actual = buscar_marco_tlb_o_memoria(pagina_actual); 
            
            direccion_fisica_actual = marco_actual * tam_pagina;

            bytes_en_este_marco = bytes(direccion_fisica_actual, bytes_leidos, tamanio_lectura);

            pedido_lectura_mmu(direccion_fisica_actual, bytes_en_este_marco);

            void* valor_leido_en_este_marco = malloc(bytes_en_este_marco);
            valor_leido_en_este_marco = recibir_resultado_mov_in();
            memcpy(contenido_leido_total + bytes_leidos, valor_leido_en_este_marco + bytes_leidos, bytes_en_este_marco);
            free(valor_leido_en_este_marco);
            
            bytes_leidos += bytes_en_este_marco;
        }
    }

    char valor_como_char = *((char*)contenido_leido_total);
    uint32_t valor = (uint32_t)valor_como_char;

    log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %u \n", contexto_ejecucion->pid, "LEER", direccion_fisica_pedida_para_log, valor);

    return contenido_leido_total;
}*/

static void* recibir_resultado_mov_in(){
    t_paquete *paquete = recibir_paquete(socket_cliente_memoria);
    void *stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == RESULTADO_MOV_IN){
        
        void* dato_a_guardar = sacar_bytes_de_paquete(&stream);

        eliminar_paquete(paquete);

        return dato_a_guardar;
    }
    else{
        log_error(cpu_logger, "No me enviaste el resultado :( \n");
        abort();
    }
    eliminar_paquete(paquete);
}




/*

//================================================== SPLIT DE DIRECCIONES FISICAS ==============================================================


//ESTO PONERLO EN UTILS!!!
typedef struct {
    //int pid; 
    uint32_t direccion_fisica;
    uint32_t tamanio;
} t_acceso_memoria;

//ME DA TODAS LAS DFS QUE NECESITO Y CUANTO ESCRIBIR EN CADA UNA PARA UNA ACCION (ESCRITURA-LECTURA) EN MEMORIA
t_list* obtener_direcciones_fisicas_mmu(uint32_t tamanio_total, uint32_t direccion_logica_inicial){

    uint32_t marco_actual = -1;
    uint32_t bytes_cargados = 0;

    t_list* lista_accesos_memoria = list_create();

    //PRIMERA PAGINA/MARCO
    uint32_t pagina_actual = floor(direccion_logica_inicial / tam_pagina);
    uint32_t direccion_fisica_actual = traducir_de_logica_a_fisica(direccion_logica_inicial);
    uint32_t bytes_en_este_marco = bytes(direccion_fisica_actual, bytes_cargados, tamanio_total);

    t_acceso_memoria* acceso = malloc(sizeof(t_acceso_memoria));
    acceso->direccion_fisica = direccion_fisica_actual;
    acceso->tamanio = bytes_en_este_marco;

    list_add(lista_accesos_memoria, acceso);

    bytes_cargados += bytes_en_este_marco;


    // PAGINAS/MARCOS SIGUIENTES SI ES QUE HAY

    if (bytes_cargados < tamanio_total){
        //EL DATO DEBERA PARTIRSE, PORQUE TAMAÑO ES MAYOR A LA PAGINA, O HAY DESPLAZAMIENTO, ETC.

        for (int i=1; bytes_cargados < tamanio_total; i++){

            pagina_actual ++;
            marco_actual = buscar_marco_tlb_o_memoria(pagina_actual); 
            
            //Las dfs siguientes al primer marco comenzaran todas en desplazamiento=0 porque se busca completar el dato entero
            direccion_fisica_actual = marco_actual * tam_pagina; 
            bytes_en_este_marco = bytes(direccion_fisica_actual, bytes_cargados, tamanio_total);

            acceso->direccion_fisica = direccion_fisica_actual;
            acceso->tamanio = bytes_en_este_marco;

            list_add(lista_accesos_memoria, acceso);

            bytes_cargados += bytes_en_este_marco;
        }


    }


    free(acceso);

    return lista_direcciones_fisicas;
}

//NUEVA ESCRITURA ESPERO QUE FINAL LPM
void escritura_en_memoria(void* contenido, t_list* lista_accesos_memoria){

    //RECORRO TODAS LAS DFS QUE HAYA EN LA LISTA QUE ME HIZO PREVIAMENTE LA MMU Y COPIO EN MEMORIA

    //uint32_t primera_df = ((t_acceso_memoria*) list_get(lista_accesos_memoria, 0))->direccion_fisica;

    uint32_t escritura_ok;
    char* valor_escrito;
    uint32_t tamanio_copiado_actual = 0;


    t_list_iterator* iterator = list_iterator_create(lista_accesos_memoria);
    while (list_iterator_has_next(iterator)) {

        t_acceso_memoria* acceso = (t_acceso_memoria*) list_iterator_next(iterator);
        
        void* contenido_a_escribir_actual = malloc(acceso->tamanio);

        memcpy(contenido_a_escribir_actual, contenido + tamanio_copiado_actual, acceso->tamanio);


        pedido_escritura_mmu(contenido_a_escribir_actual, acceso->direccion_fisica, acceso->tamanio);
        recv(socket_cliente_memoria, &escritura_ok, sizeof(uint32_t), MSG_WAITALL);

        if (escritura_ok == 1){
            
            tamanio_copiado_actual += acceso->tamanio; 

            //MEDIO AL DOPE LOGGEAR CADA PARTE NO?
            //char *valor_escrito = (char *) contenido_a_escribir_actual; //?????? se muestra asi o como? ver para el log
            //log_info(cpu_logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d \n", contexto_ejecucion->pid, Acceso->direccion_fisica, valor_para_log); //VER EL CASTEO DE VOID* A CHARS Y A INT
        
        }else{
            //log_error(...)
        }
        
        free(acceso);
        
    }
    list_iterator_destroy(iterator);
    
    //char* valor_log = (char *) contenido_a_escribir_actual; //?????? 
    //log_info(cpu_logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %d - Valor: %d \n", contexto_ejecucion->pid, primera_df, valor_para_log); //VER EL CASTEO DE VOID* A CHARS Y A INT
        


    //CUANDO SE TERMINA DE HACER LA ESCRITURA, HAY QUE MATAR LA LISTA DE DFS SINO QUEDA MEMORIA VOLANDO EN CUALQUIER LADO
    //TODO : HACER LO MISMO CUANDO SE HAGA ESCRITURA/LECTURA EN IO
    list_destroy_and_destroy_elements(lista_accesos_memoria, free);

}

//ESTE METODO MEPA QUE HAY QUE USARLO EN IO TAMBIEN, LO PODEMOS MOVER A UTILS
static uint32_t calcular_tamanio_contenido(t_list* list_accesos_memoria) {
    uint32_t tamanio_total = 0;
    t_list_iterator* iterator = list_iterator_create(list_accesos_memoria);
    while (list_iterator_has_next(iterator)) {
        t_acceso_memoria* acceso= (t_acceso_memoria*)list_iterator_next(iterator);
        tamanio_total += elemento->tamanio;
        free(acceso);
    }
    list_iterator_destroy(iterator);
    return tamanio_total;
}

static void* recibir_lectura_de_memoria(){
    t_paquete *paquete = recibir_paquete(socket_cliente_memoria);
    void *stream = paquete->buffer->stream;
    if (paquete->codigo_operacion == RESULTADO_LECTURA){
        void* dato_a_guardar = sacar_bytes_de_paquete(&stream);
        eliminar_paquete(paquete);
        return dato_a_guardar;
    }
    else{
        log_error(cpu_logger, "No se recibio lectura de Memoria en Cpu \n");
        abort();
    }
    eliminar_paquete(paquete);
}

//NUEVA LECTURA ESPERO QUE FINAL LPM
void* lectura_en_memoria(t_list* lista_accesos_memoria){

    uint32_t tamanio_total = calcular_tamanio_contenido(lista_accesos_memoria);
    //uint32_t primera_df = ((t_acceso_memoria*) list_get(lista_accesos_memoria, 0))->direccion_fisica;

    uint32_t escritura_ok;
    char* valor_escrito;
    uint32_t tamanio_leido_actual = 0;
    
    void* contenido_leido_total = malloc(tamanio_total); 


    t_list_iterator* iterator = list_iterator_create(lista_accesos_memoria);
    while (list_iterator_has_next(iterator)) {

        t_acceso_memoria* acceso = (t_acceso_memoria*) list_iterator_next(iterator);
        
        pedido_lectura_mmu(acceso->direccion_fisica, acceso->tamanio);

        void* valor_leido = malloc(acceso->tamanio);
        valor_leido = recibir_lectura_de_memoria();
        memcpy(contenido_leido_total + tamanio_leido_actual, valor_leido, acceso->tamanio);

        //char* valor_log = (char *) valor_leido; //?????? 
        //log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %u \n", contexto_ejecucion->pid, "LEER", primera_df, valor_log);

        free(valor_leido);
        

        tamanio_leido_actual += acceso->tamanio;
        
        free(acceso);
        
    }
    list_iterator_destroy(iterator);
    
    //CUANDO SE TERMINA DE HACER LA LECTURA, HAY QUE MATAR LA LISTA DE DFS SINO QUEDA MEMORIA VOLANDO EN CUALQUIER LADO
    list_destroy_and_destroy_elements(lista_accesos_memoria, free);


    //char* valor_log = (char *) contenido_leido_total; //?????? 
    //log_info(cpu_logger, "PID: %d - Accion: %s - Direccion Fisica: %d - Valor: %u \n", contexto_ejecucion->pid, "LEER", primera_df, valor_log);

    return contenido_leido_total;
}

*/