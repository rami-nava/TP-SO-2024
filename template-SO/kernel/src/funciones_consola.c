#include "kernel.h"

// MULTIPROGRAMACION //
void consola_modificar_multiprogramacion(int nuevo_valor) {
        int grado_anterior = config_valores_kernel.grado_multiprogramacion;

        sem_destroy(&grado_multiprogramacion);

        sem_init(&grado_multiprogramacion, 0, nuevo_valor);
        printf("Grado Anterior: %d - Grado Actual: %d \n", grado_anterior, nuevo_valor);
}

// PROCESO_ESTADO //
void consola_proceso_estado() {
    listar_PIDS(cola_NEW);
    listar_PIDS(cola_READY);
}
