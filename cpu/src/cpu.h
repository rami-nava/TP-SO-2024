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

extern arch_config config_valores_cpu;

//Variables
extern t_config* config;
extern t_log* cpu_logger ;
extern int socket_cliente_memoria;
extern int socket_servidor_dispatch;
extern int socket_servidor_interrupt;
extern int socket_cliente_dispatch;
extern int socket_cliente_interrupt;
extern int tam_pagina;
extern bool hay_page_fault;
extern int instruccion_actual;


// FUNCIONES
void cargar_configuracion(char *path);
void atender_dispatch();
void atender_interrupt();
void mov_in(char* registro, char* direccion_logica);
void mov_out(char* direccion_logica, char* registro);
void setear_registro(char* registro, char* valor);
bool no_es_bloqueante(codigo_instrucciones instruccion_actual);
void ciclo_de_instruccion();
uint32_t buscar_registro(char*registro);




#endif 