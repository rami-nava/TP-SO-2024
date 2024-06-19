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
#include <commons/collections/queue.h>

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

typedef struct {
	int pid;
   uint32_t pagina;
   uint32_t marco; 
   int ultimo_uso;
   int tiempo_carga;
} t_entrada;

extern arch_config config_valores_cpu; 


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

extern t_list* tlb;
extern int cantidad_entradas_tlb;
extern char* algoritmo_tlb;
extern uint32_t tam_pagina;


// FUNCIONES
void cargar_configuracion(char *path);
void atender_dispatch();
void atender_interrupt(void * socket_servidor_interrupt);
void setear_registro(char* registro, char* valor);
void setear_registro_entero(char* registro, uint32_t valor);
bool no_es_bloqueante(codigo_instrucciones instruccion_actual);
void ciclo_de_instruccion();
uint32_t buscar_registro(char*registro);
void inicializar_semaforos();
void modificar_motivo (codigo_instrucciones comando, int cantidad_parametros, char* parm1, char* parm2, char* parm3, char* parm4, char* parm5);
void* buscar_valor_registro_generico(char* registro);
uint32_t tamanio_registro(char* registro);

// MMU
uint32_t traducir_de_logica_a_fisica(uint32_t direccion_logica);
t_list* obtener_direcciones_fisicas_mmu(uint32_t tamanio_total, uint32_t direccion_logica_inicial);
void cargar_direcciones_fisicas_en_contexto(uint32_t tamanio_total, uint32_t direccion_logica_inicial);
uint32_t buscar_marco_tlb_o_memoria (uint32_t numero_pagina);

//TLB
uint32_t consultar_tlb(int pid, uint32_t pagina); 
void agregar_entrada_tlb(int pid, uint32_t pagina, uint32_t marco); 
void imprimir_tlb(t_list* tlb);

// MANEJO DE MEMORIA
void realizar_handshake();
void escritura_en_memoria(void* contenido, t_list* lista_accesos_memoria);
void* lectura_en_memoria(uint32_t tamanio_total, t_list* lista_accesos_memoria);
void mov_in(char* registro, char* direccion_logica);
void mov_out(char* direccion_logica, char* registro);
void resize(char* tamanio);
void copy_string(char* tamanio);


#endif 