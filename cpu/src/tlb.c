#include "cpu.h"

//BASE TLB TODAVIA NO AGREGO NADA

//config_valores_cpu.cantidad_entradas_tlb 
//si son 0 esta desabilitada, siempre es un entero

//config_valores_cpu.algoritmo_tlb


/*void imprimir_tlb(t_list *tlb) {
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
}*/