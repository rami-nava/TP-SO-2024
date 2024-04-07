#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <CUnit/CUnit.h>
#include <sys/types.h>
#include <commons/log.h>
#include <pthread.h>
#include "socket.h"
#include "operaciones.h"

// DEFINICIONES 
//#define MAX_CHAR 100

//VARIABLES GLOBALES
extern t_log* memoria_logger;
extern t_config* config;
extern int socket_memoria;
extern int server_fd;
extern void* espacio_usuario;
extern t_list* procesos_en_memoria;
extern int socket_fs;
extern sem_t solucionado_pf;
extern sem_t swap_creado;
extern sem_t swap_finalizado;
extern pthread_mutex_t mutex_path;
extern pthread_mutex_t mutex_instrucciones;
extern pthread_mutex_t mutex_lista_instrucciones;
extern pthread_mutex_t mutex_tiempo;

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

typedef struct {
	int id;
    int numero_de_pagina;
    int marco; 
	bool ocupado;
    int bit_de_presencia; 
    int bit_modificado; 
    int posicion_swap; 
	int tiempo_uso;
	int tiempo_de_carga;
} t_pagina;

typedef struct {
    bool ocupado;
} t_marco;

typedef struct {
    t_marco* marcos;
	int cantidad_marcos;
} t_memoria_principal;

extern t_memoria_principal memoria;
typedef struct 
{
	int pid;
	t_list* paginas_en_memoria;
	t_list* instrucciones;              
	t_list* bloques_reservados;
	int cantidad_entradas;                 
} t_proceso_en_memoria;


//======================================================= FUNCIONES =========================================================================================================
/// CONEXIONES y CONFIG ///
int atender_clientes_memoria(int);
void cargar_configuracion(char* );
void inicializar_semaforos();

/// @brief CPU + INSTRUCCIONES ///
t_proceso_en_memoria* crear_estructuras_memoria(int pid, FILE* archivo);
char* buscar_instruccion_proceso(int program_counter, int pid);
t_proceso_en_memoria *buscar_proceso(t_list *lista, int pid_buscado);
void desocupar_marco(int nro_marco);
void enviar_respuesta_pedido_marco(int socket_cpu, uint32_t num_pagina, int pid);

/// @brief ESPACIO USUARIO ///
void creacion_espacio_usuario();
void liberar_espacio_usuario() ;
void escribir(int* valor, uint32_t direccion_fisica, uint32_t direccion_logica, int pid, int socket_cpu);
uint32_t leer(uint32_t direccion_fisica, uint32_t direccion_logica, int pid);
void escribir_en_memoria(void* contenido, size_t tamanio_contenido, uint32_t direccion_fisica);
void* leer_en_memoria(size_t tamanio_contenido, uint32_t direccion_fisica);
void enviar_valor_de_lectura(uint32_t valor, int socket_cpu);

/// @brief  TABLAS DE PAGINAS ///
int buscar_marco(int pid, int num_pagina);
void inicializar_la_tabla_de_paginas(int tamanio_memoria, int tamanio_pagina);
void inicializar_swap_proceso(int pid_proceso, int cantidad_paginas_proceso);
void crear_tablas_paginas_proceso(int pid, int cantidad_paginas_proceso, char* path_recibido);
void finalizar_en_memoria(int pid);
void escribir_en_memoria_principal(int nro_pagina, int posicion_swap, int pid);
void enviar_pedido_pagina_para_escritura(int pid, int pag_pf);
t_proceso_en_memoria* buscar_proceso_en_memoria(int pid);
t_pagina* buscar_pagina(int pid, int num_pagina);
int mas_vieja(t_pagina* una_pag, t_pagina* otra_pag);
int obtener_tiempo();
int obtener_tiempo_carga();
void crear_nueva_pagina(t_proceso_en_memoria* proceso_en_memoria, int nro_pagina, int posicion_swap, int marco);

/// @brief SWAP ///
void escribir_en_swap(t_pagina* pagina, int pid);
void swap_in_tabla_proceso (int nro_pagina, int pid);
void recibir_pagina_swap (int pid);
void enviar_paquete_confirmacion_escritura();
void bloques_para_escribir(int tam_bloque, void* contenido, uint32_t puntero_archivo, char* nombre_archivo);

#endif
