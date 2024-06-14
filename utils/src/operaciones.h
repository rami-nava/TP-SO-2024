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
	CREACION_ESTRUCTURAS_MEMORIA,
	CONTEXTO_ACTUALIZADO,
	INSTRUCCION_SOLICITADA,
	MANDAR_INSTRUCCION,
	HANDSHAKE,
	TRADUCIR_PAGINA_A_MARCO,
	NUMERO_MARCO,
	FINALIZAR_EN_MEMORIA,
	FINALIZAR_PROCESO,
	DESALOJO,
	GENERICA_IO_SLEEP,
	STDIN_READ,
	STDOUT_WRITE,
	REALIZAR_LECTURA,
	RESULTADO_LECTURA,
	REALIZAR_ESCRITURA,
	CREAR_ARCHIVO,
	ELIMINAR_ARCHIVO,
	TRUNCAR_ARCHIVO,
	LEER_ARCHIVO,
	ESCRIBIR_ARCHIVO,
	LEER_CONTENIDO_EN_MEMORIA,
	VALOR_LECTURA,
	INTERFAZ_GENERICA,
    INTERFAZ_STDIN,
    INTERFAZ_STDOUT,
    INTERFAZ_DIALFS,
	RESULTADO_MOV_IN,
	PEDIDO_RESIZE,
	PEDIDO_COPY_STRING,
	DESCONECTAR_IO,
	LEER_BITMAP,
	LEER_CONTENIDO_EN_MEMORIA_DESDE_CPU,
	ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_CPU,
	ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_DIALFS,
	ESCRIBIR_CONTENIDO_EN_MEMORIA_DESDE_STDIN
} op_code;

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCKED_RECURSO,
	BLOCKED_IO_GENERICA,
	BLOCKED_IO_STDIN,
	BLOCKED_IO_STDOUT,
	BLOCKED_IO_DIALFS,
	SALIDA
} estado_proceso;

typedef enum {
	SET,
	SUM,
	SUB,
	JNZ,
	WAIT,
	SIGNAL,
	MOV_IN,
	MOV_OUT,
	RESIZE,
	COPY_STRING,
	IO_GEN_SLEEP,
	IO_STDIN_READ,
	IO_STDOUT_WRITE,
	IO_FS_CREATE,
	IO_FS_DELETE,
	IO_FS_TRUNCATE,
	IO_FS_WRITE,
	IO_FS_READ,
	FIN_QUANTUM,
	EXIT,
	EXIT_MAS_FIN_QUANTUM,
	OUT_OF_MEMORY
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
	estado_proceso estado;
	t_list* recursos_asignados;
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
	int eliminado;
	t_list* direcciones_fisicas;
}t_pcb; 

typedef struct {
    uint32_t direccion_fisica;
    uint32_t tamanio;
} t_acceso_memoria;

//======================================================= CREAR PAQUETES ==========================================================================================================
t_paquete* crear_paquete(op_code );
void enviar_paquete(t_paquete* , int );
void* serializar_paquete(t_paquete* , int );
t_paquete* recibir_paquete(int );
int recibir_operacion(int socket_cliente);
void* recibir_buffer(int socket_cliente);

//======================================================= AGREGAR_A_PAQUETE =========================================================================================================
void agregar_entero_a_paquete(t_paquete* ,int );
void agregar_entero_sin_signo_a_paquete(t_paquete* , uint32_t);
void agregar_byte_sin_signo_a_paquete(t_paquete* , uint8_t);
void agregar_cadena_a_paquete(t_paquete* , char* );
void agregar_array_cadenas_a_paquete(t_paquete* , char** );
void agregar_lista_de_cadenas_a_paquete(t_paquete* , t_list*);
void agregar_puntero_a_paquete(t_paquete* , void* , uint32_t);
void agregar_a_paquete(t_paquete* , void* , int );
void agregar_bytes_a_paquete(t_paquete* , void* , uint32_t);
void agregar_lista_de_accesos_a_paquete(t_paquete* paquete, t_list* accesos);

//======================================================= SACAR_DE_PAQUETE =========================================================================================================
char* sacar_cadena_de_paquete(void** );
int sacar_entero_de_paquete(void** );
uint32_t sacar_entero_sin_signo_de_paquete(void** );
uint8_t sacar_byte_sin_signo_de_paquete(void** );
char** sacar_array_cadenas_de_paquete(void** );
t_list* sacar_lista_de_cadenas_de_paquete(void**);
void* sacar_puntero_de_paquete(void** );
void* sacar_bytes_de_paquete(void**);
t_list* sacar_lista_de_accesos_de_paquete(void** stream) ;
//======================================================= LIBERAR_MEMORIA ==============================================================================================================
void eliminar_paquete(t_paquete* );
void free_array (char ** );

#endif
