#include "memoria.h"


//================================================= Handshake =====================================================================
void enviar_paquete_handshake(int socket_cliente) {

    int tam_pagina = config_valores_memoria.tam_pagina;

	t_paquete* handshake=crear_paquete(HANDSHAKE);
	agregar_entero_a_paquete(handshake,tam_pagina);

	enviar_paquete(handshake,socket_cliente);
}

//=============================================== INSTRUCCIONES ================================================================
void leer_instrucciones_desde_archivo(t_proceso_en_memoria* proceso, FILE* archivo) 
{
    char *instruccion = NULL;
    size_t longitud = 0;

    while (getline(&instruccion, &longitud, archivo) != -1)
    {
        if (strcmp(instruccion, "\n"))
        {
            int longitud = strlen(instruccion);
            if (instruccion[longitud - 1] == '\n')
                instruccion[longitud - 1] = '\0';

            char* aux = malloc(strlen(instruccion) + 1);
            strcpy(aux, instruccion);
            list_add(proceso->instrucciones, aux);
        }
    }
    free(instruccion);
    fclose(archivo);
}

char* buscar_instruccion_proceso(uint32_t PC, int pid)
{
	//Obtengo el proceso por el pid
	t_proceso_en_memoria *proceso = buscar_proceso(procesos_en_memoria, pid);

	//Busco la instruccion en la lista segun el PC
	char *instruccion = list_get(proceso->instrucciones, PC);
    
	return instruccion;
}