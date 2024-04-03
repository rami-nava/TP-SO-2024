#include "contexto.h"

t_contexto* contexto_ejecucion = NULL;

void iniciar_contexto(){

    contexto_ejecucion = malloc(sizeof(t_contexto));
	contexto_ejecucion->instrucciones = list_create();
	contexto_ejecucion->cantidad_instrucciones = 0;
	contexto_ejecucion->pid = 0;
	contexto_ejecucion->program_counter = 0;
	contexto_ejecucion->AX = 0;
    contexto_ejecucion->BX = 0;
    contexto_ejecucion->CX = 0;
    contexto_ejecucion->DX = 0;
    contexto_ejecucion->motivo_desalojo = (t_motivo_de_desalojo *)malloc(sizeof(t_motivo_de_desalojo));
    contexto_ejecucion->motivo_desalojo->parametros[0] = "";
    contexto_ejecucion->motivo_desalojo->parametros[1] = "";
    contexto_ejecucion->motivo_desalojo->parametros[2] = "";
    contexto_ejecucion->motivo_desalojo->cantidad_parametros = 0;
    contexto_ejecucion->motivo_desalojo->comando = 0;
	
}

//================================== ENVIAR/RECIBIR CONTEXTO ================================================================
void enviar_contexto(int socket_cliente) {
    t_paquete* paquete = crear_paquete(CONTEXTO);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->pid);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->program_counter);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->AX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->BX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->CX);
    agregar_entero_sin_signo_a_paquete(paquete, contexto_ejecucion->DX);
    agregar_entero_a_paquete(paquete, contexto_ejecucion->motivo_desalojo->cantidad_parametros);
    for (int i = 0; i < contexto_ejecucion->motivo_desalojo->cantidad_parametros; i++) {
        agregar_cadena_a_paquete(paquete, contexto_ejecucion->motivo_desalojo->parametros[i]);
    }
    agregar_entero_a_paquete(paquete, contexto_ejecucion->motivo_desalojo->comando);

    enviar_paquete(paquete, socket_cliente);
}

void recibir_contexto(int socket_cliente) {
    t_paquete* paquete = recibir_paquete(socket_cliente);
    void *stream = paquete->buffer->stream;

    contexto_ejecucion->pid = sacar_entero_de_paquete(&stream);
    contexto_ejecucion->program_counter = sacar_entero_de_paquete(&stream);
    contexto_ejecucion->AX = sacar_entero_sin_signo_de_paquete(&stream);
    contexto_ejecucion->BX = sacar_entero_sin_signo_de_paquete(&stream);
    contexto_ejecucion->CX = sacar_entero_sin_signo_de_paquete(&stream);
    contexto_ejecucion->DX = sacar_entero_sin_signo_de_paquete(&stream);
    contexto_ejecucion->motivo_desalojo->cantidad_parametros = sacar_entero_de_paquete(&stream);
    for (int i = 0; i < contexto_ejecucion->motivo_desalojo->cantidad_parametros; i++) {
        contexto_ejecucion->motivo_desalojo->parametros[i] = sacar_cadena_de_paquete(&stream);
        }

    eliminar_paquete(paquete);
}
//================================== LIBERAR MEMORIA ================================================================
void liberar_memoria_contexto() {
    list_destroy_and_destroy_elements(contexto_ejecucion->instrucciones, free);
    for (int i = 0; i < contexto_ejecucion->motivo_desalojo->cantidad_parametros; i++) 
        if (strcmp(contexto_ejecucion->motivo_desalojo->parametros[i], "")) free(contexto_ejecucion->motivo_desalojo->parametros[i]);
    free(contexto_ejecucion->motivo_desalojo);
    free(contexto_ejecucion);
    contexto_ejecucion = NULL;
}

void liberar_memoria_contexto_unico() {
    list_destroy(contexto_ejecucion->instrucciones);
    for (int i = 0; i < contexto_ejecucion->motivo_desalojo->cantidad_parametros; i++) 
        if (strcmp(contexto_ejecucion->motivo_desalojo->parametros[i], "")) free(contexto_ejecucion->motivo_desalojo->parametros[i]);
    free(contexto_ejecucion->motivo_desalojo);
    free(contexto_ejecucion);
    contexto_ejecucion = NULL;
}