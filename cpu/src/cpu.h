#ifndef CPU_H
#define CPU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <CUnit/CUnit.h>
#include <sys/types.h>
#include <commons/log.h>
#include <pthread.h>
#include "socket.h"
#include "operaciones.h"
#include "contexto.h"
#include "stack.h"


// Estructuras //
typedef struct  
{
   char* ip_cpu;
   char* ip_memoria;
   char* puerto_memoria;
   char* puerto_escucha_dispatch;
   char* puerto_escucha_interrupt;
   int cantidad_entradas_tlb;
   char* algoritmo_tlb;
} arch_config;

extern arch_config config_valores_cpu; //PUEDE QUE ESTO AL FINAL NO SE USA? se declara en cpu_utils puede confudirse lo mismo el t_config, y lo mismo pasa en la memoria.


//Variables
extern t_config* config;
extern t_log* cpu_logger ;
extern int socket_cliente_memoria;
extern int socket_servidor_dispatch;
extern int socket_servidor_interrupt;
extern int socket_cliente_dispatch;
extern int socket_cliente_interrupt;
extern int instruccion_actual;
extern bool seguir_ejecutando;

extern pthread_mutex_t interrupcion_mutex;
extern pthread_mutex_t seguir_ejecutando_mutex;


// FUNCIONES
void cargar_configuracion(char *path);
void atender_dispatch();
void atender_interrupt(void * socket_servidor_interrupt);
void mov_in(char* registro, char* direccion_logica);
void mov_out(char* direccion_logica, char* registro);
void resize(char* tamanio);
void copy_string(char* tamanio);
void setear_registro(char* registro, char* valor);
bool no_es_bloqueante(codigo_instrucciones instruccion_actual);
void ciclo_de_instruccion();
uint32_t buscar_registro(char*registro);
void inicializar_semaforos();
void realizar_handshake();
uint32_t traducir_de_logica_a_fisica(uint32_t direccion_logica);


//TLB

/* La idea es:
   cada vez que la cpu necesita traducir una DL = (PAG-DESP)
   CONSULTA A TLB -> TLB HIT : -CPU OBTIENE MARCO, CALCULA DF Y ACCEDE A MEMORIA 
                               -TLB AJUSTA LISTAS SEGUN ALGORITMO
                  -> TLB MISS : -CPU OBTIENE -1, PIDE A MEMORIA MARCO, CALCULA DF, ACCEDE A MEMORIA
                                -PIDE A TLB AGREGAR LA ENTRADA (PID-PAG-MARCO)
                                -TLB AJUSTA LISTAS SEGUN ALGORITMO
*/

/*
typedef struct {
	int pid;
   int pagina;
   int marco; 
} t_entrada;



void crear_tlb(char * algoritmo_sustitucion, char * cantidad_entradas);

int consultar_tlb(int pid, int pagina); //HIT: marco - MISS: -1
void agregar_entrada_tlb(int pid, int pagina, int marco); 
void imprimir_tlb(t_list *tlb);
*/



#endif 