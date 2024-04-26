#include "cpu.h"

//TLB -> LRU -> MANEJA QUEUE PERO AL CONSULTAR UNA PAGINA SE SACA Y SE ENCOLA NUEVAMENTE
//TLB -> FIFO -> MANEJA QUEUE

//Si cant_entradas es 0 esta desabilitada, siempre es un entero


int consultar_tlb(int pid, int pagina){

    int tamanio_actual_tlb = queue_size(tlb);

    //HIT: MARCO
    for (int i = 0; i < tamanio_actual_tlb; i++) {
        
        t_entrada* entrada_actual = (t_entrada*) list_get(tlb->elements, i);

        if (entrada_actual->pid == pid && entrada_actual->pagina == pagina) {

            //Uso la entrada, por ende en FIFO no pasa nada, en LRU debo mover la entrada al tope de la cola
            if(algoritmo_tlb == "LRU"){
                
                list_remove(tlb->elements, i);
                queue_push(tlb, entrada_actual);
            }
            
            return entrada_actual->marco;
        }

    }

    //MISS: -1
    return -1;

} 

void agregar_entrada_tlb(int pid, int pagina, int marco){

    int tamanio_actual_tlb = list_size(tlb);

    t_entrada* nueva_entrada = (t_entrada*) malloc(sizeof(t_entrada));
    nueva_entrada->pid = pid;
    nueva_entrada->pagina = pagina;
    nueva_entrada->marco = marco;


    if( (tamanio_actual_tlb < cantidad_entradas_tlb) ){

        //No importa el algoritmo siempre encolo
        queue_push(tlb, nueva_entrada);

    }else{
        
        if( (cantidad_entradas_tlb != 0) ){

            //REEMPLAZO DE ENTRADA
            //En ambos casos debo desencolar la primera porque el manejo del algortimo de hace desde la consulta de entradas
            
            queue_pop(tlb);
            queue_push(tlb, nueva_entrada);

        }else{
            //TODO SI NO ESTA HABILITADA QUE PASA? LOG U OTRA COSA?
        }
    }

  
    

   

}



void imprimir_tlb(t_list *tlb) { //SOLO PARA TESTING
    
    if (tlb == NULL) {
        printf("TLB vacía\n");
        return;
    }

    t_list_iterator *iterator = list_iterator_create(tlb);
    
    int index = 0;

    while (list_iterator_has_next(iterator)) {
        
        t_entrada *entrada = list_iterator_next(iterator);
        printf("Posicion TLB: %d - PID: %d - Pagina: %d - Marco: %d\n", index, entrada->pid, entrada->pagina, entrada->marco);
        index++;
    }

    list_iterator_destroy(iterator);
}