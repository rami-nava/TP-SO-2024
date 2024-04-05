#ifndef OPERACIONES_H_
#define OPERACIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include "semaphore.h"
#include "inttypes.h"

//==================================================== Estructuras =========================================================================================================
typedef enum{
	HANDSHAKE
} op_code;

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCKED,
	EXIT
} estado_proceso;

typedef enum{
	FIFO,
	PRIORIDADES,
	RR
} algoritmo;

typedef enum {
	SET,
	SUM,
	SUB,
	JNZ,
	SLEEP,
	CONTEXTO,
	READ,
	VALOR_READ,
	TRADUCIR_PAGINA_A_MARCO,
	NUMERO_MARCO,
	MANDAR_INSTRUCCIONES,
	CREACION_ESTRUCTURAS_MEMORIA,
	FINALIZAR_EN_MEMORIA,
	CONTEXTO_ACTUALIZADO,
	WRITE,
	WAIT,
	SIGNAL,
	MOV_IN,
	MOV_OUT,
	F_OPEN,
	F_CLOSE,
	F_SEEK,
	F_READ,
	F_WRITE,
	F_TRUNCATE,
	INSTRUCCION_EXIT
} codigo_instrucciones;

typedef struct{
	int size;
	void* stream;
} t_buffer;

typedef struct{
    op_code codigo_operacion;
    t_buffer *buffer;
}t_paquete;

typedef struct{
	char* puerto;
	char* ip;
}conexion_t;


//======================================================= PCB ==============================================================================================================
typedef struct{
	int pid;
	int program_counter;
	estado_proceso estado;
	char* path_proceso;
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
	int quantum;
}t_pcb; 


//======================================================= CREAR PAQUETES ==========================================================================================================
t_paquete *crear_paquete_con_codigo_de_operacion(uint8_t codigo);
t_paquete* crear_paquete(op_code );
void crear_buffer(t_paquete *paquete);
void enviar_paquete(t_paquete* , int );
void* serializar_paquete(t_paquete* , int );

//======================================================= SEND/RECV ======================================================================================================
int enviar_datos(int , void *, uint32_t );
int recibir_datos(int , void *, uint32_t );
void *recibir_stream(int *size, int socket_cliente);
t_paquete* recibir_paquete(int );
//======================================================= AGREGAR_A_PAQUETE =========================================================================================================
void agregar_entero_a_paquete(t_paquete* ,int );
void agregar_entero_sin_signo_a_paquete(t_paquete* , uint32_t);
void agregar_cadena_a_paquete(t_paquete* , char* );
void agregar_array_cadenas_a_paquete(t_paquete* , char** );
void agregar_lista_de_cadenas_a_paquete(t_paquete* , t_list*);
void agregar_puntero_a_paquete(t_paquete* , void* , uint32_t);
void agregar_a_paquete(t_paquete* , void* , int );
void agregar_bytes_a_paquete(t_paquete* , void* , uint32_t);

//======================================================= SACAR_DE_PAQUETE =========================================================================================================
char* sacar_cadena_de_paquete(void** );
int sacar_entero_de_paquete(void** );
uint32_t sacar_entero_sin_signo_de_paquete(void** );
char** sacar_array_cadenas_de_paquete(void** );
t_list* sacar_lista_de_cadenas_de_paquete(void**);
void* sacar_puntero_de_paquete(void** );
void* sacar_bytes_de_paquete(void** , uint32_t );

//======================================================= LIBERAR_MEMORIA ==============================================================================================================
void eliminar_paquete(t_paquete* );
void free_lista(t_list* lista);
void free_array (char ** );

#endif
