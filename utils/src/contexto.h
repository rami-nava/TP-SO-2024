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
    t_motivo_de_desalojo* motivo_desalojo;
    uint32_t PC;
    uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
    uint32_t SI;
    uint32_t DI;
} t_contexto;

extern t_contexto* contexto_ejecucion;
extern int socket_cliente;

void iniciar_contexto();
void enviar_contexto(int socket_cliente);
void recibir_contexto_cpu(t_paquete* paquete, void* stream);
void recibir_contexto(int socket_cliente);
void liberar_memoria_contexto();

#endif
