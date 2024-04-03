#ifndef CONTEXTO_H_
#define CONTEXTO_H_
 
 #include <commons/collections/list.h>
 #include "operaciones.h"

typedef struct {
	codigo_instrucciones comando; 
	int cantidad_parametros;
	char* parametros[3]; 
} t_motivo_de_desalojo; 

typedef struct {
    int pid; 
    int program_counter;
	int cantidad_instrucciones;
    t_list* instrucciones; 
    t_motivo_de_desalojo* motivo_desalojo;
    uint32_t AX;
    uint32_t BX;
    uint32_t CX;
    uint32_t DX;
} t_contexto;

extern t_contexto* contexto_ejecucion;
extern int socket_cliente;

void iniciar_contexto();
void enviar_contexto(int socket_cliente);
void recibir_contexto(int socket_cliente);
void liberar_memoria_contexto();
void liberar_memoria_contexto_unico();

#endif
