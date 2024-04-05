#include "kernel.h"

static t_pcb *proximo_a_ejecutar_FIFO();

void planificador_corto_plazo_segun_algoritmo() {
    char *algoritmo = config_valores_kernel.algoritmo;

    if (!strcmp(algoritmo, "FIFO"))
    {
        planificador_corto_plazo(proximo_a_ejecutar_FIFO);
    }
    else if (!strcmp(algoritmo, "XX"))
    {
        //planificador_corto_plazo(proximo_a_ejecutar_XX);
    }
    else
    {
        log_error(kernel_logger, "Algoritmo invalido. Debe ingresar FIFO");
        abort();
    }
}

static t_pcb *proximo_a_ejecutar_FIFO(){
    return desencolar(cola_READY);
}

 //  Funciones de algoritmo XX //