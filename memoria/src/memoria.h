#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <CUnit/CUnit.h>
#include <sys/types.h>
#include <pthread.h>

#include <commons/log.h>

#include "socket.h"
#include "operaciones.h"


//VARIABLES GLOBALES
extern t_log* memoria_logger;
extern t_config* config;
extern int socket_memoria;
extern int server_fd;


extern void* espacio_usuario; //Espacio contiguo
extern t_list* procesos_en_memoria; //Lista de t_proceso_en_memoria con las tablas de paginas e instrucciones para cada proceso
extern t_list* marcos; //Lista de los t_marco y su info
extern int cantidad_marcos;


//ESTRUCTURAS
typedef struct  
{
	char* ip_memoria;
	char* puerto_escucha;
	char* ip_filesystem;
	char* puerto_filesystem;
	int tam_memoria;
	int tam_pagina;
	char* path_instrucciones;	
	int retardo_respuesta;
	char* algoritmo_reemplazo;
} arch_config;
extern arch_config config_valores_memoria;

//TODO cambiar todos los ints por uint32_T y poner nombres regulares no: numero_de_pagina y en el otro nro_pag

typedef struct {
	int pid;
    int numero_de_pagina;
    int marco; 
	bool cargada_en_memoria;
} t_pagina;

typedef struct {
    int nro_marco;
	int nro_pag;
    int pid_proceso;
	bool libre;
	int cantidad_bytes_libres;
} t_marco;


typedef struct 
{
	int pid;
	t_list* paginas_en_memoria;
	t_list* instrucciones;                              
} t_proceso_en_memoria;





//======================================================= FUNCIONES =========================================================================================================
/// UTILS ///
int atender_clientes_memoria(int socket);
void cargar_configuracion(char* path);
t_proceso_en_memoria* buscar_proceso(t_list *lista, int pid_buscado);
void finalizar_en_memoria(int pid);

/// @brief CPU + INSTRUCCIONES ///
void enviar_paquete_handshake(int socket_cliente);
char* buscar_instruccion_proceso(uint32_t PC, int pid);
void leer_instrucciones_desde_archivo(t_proceso_en_memoria* proceso, FILE* archivo);


/// @brief PETICIONES DE IO ///
void realizar_lectura(uint32_t direccion_fisica, uint32_t tamanio_lectura, int cliente);
void realizar_escritura(uint32_t direccion_fisica, char* texto_a_guardar, uint32_t tamanio_a_guardar);

/// @brief ESPACIO USUARIO ///
void creacion_espacio_usuario();


/// @brief  PROCESOS EN MEMORIA - MARCOS ///
void crear_estructuras_memoria(int pid, FILE* archivo);
void crear_marcos_memoria();


//INTRUCCIONES CPU ///
int resize(uint32_t pid, uint32_t tamanio);


// FUNCIONES MARCOS/PAGINAS ///
int cantidad_de_marcos_libres();
int cantidad_de_marcos_necesarios(int tamanio);
void asignar_marcos_a_proceso(uint32_t pid, int cantidad_de_marcos);


/*

int cantidad_de_marcos_del_proceso(t_proceso_en_memoria* proceso);
t_proceso_en_memoria* obtener_proceso_en_memoria(uint32_t pid);
*/

#endif


