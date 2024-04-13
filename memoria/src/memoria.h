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
extern void* espacio_usuario;
extern t_list* procesos_en_memoria;


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
/// UTILS ///
int atender_clientes_memoria(int socket);
void cargar_configuracion(char* path);
t_proceso_en_memoria* buscar_proceso(t_list *lista, int pid_buscado);
void finalizar_en_memoria(int pid);

/// @brief CPU + INSTRUCCIONES ///
void enviar_paquete_handshake(int socket_cliente);
char* buscar_instruccion_proceso(uint32_t PC, int pid);
void leer_instrucciones_desde_archivo(t_proceso_en_memoria* proceso, FILE* archivo);
void desocupar_marco(int nro_marco);
void enviar_respuesta_pedido_marco(int socket_cpu, uint32_t num_pagina, int pid);

/// @brief ESPACIO USUARIO ///
void creacion_espacio_usuario();
void escribir(int* valor, uint32_t direccion_fisica, uint32_t direccion_logica, int pid, int socket_cpu);
uint32_t leer(uint32_t direccion_fisica, uint32_t direccion_logica, int pid);
void escribir_en_memoria(void* contenido, size_t tamanio_contenido, uint32_t direccion_fisica);
void* leer_en_memoria(size_t tamanio_contenido, uint32_t direccion_fisica);
void enviar_valor_de_lectura(uint32_t valor, int socket_cpu);

/// @brief  TABLAS DE PAGINAS ///
void crear_estructuras_memoria(int pid, FILE* archivo);
int buscar_marco(int pid, int num_pagina);
void inicializar_la_tabla_de_paginas(int tamanio_memoria, int tamanio_pagina);
void crear_tablas_paginas_proceso(int pid, int cantidad_paginas_proceso, char* path_recibido);
void escribir_en_memoria_principal(int nro_pagina, int posicion_swap, int pid);
void enviar_pedido_pagina_para_escritura(int pid, int pag_pf);
t_pagina* buscar_pagina(int pid, int num_pagina);
int mas_vieja(t_pagina* una_pag, t_pagina* otra_pag);
int obtener_tiempo();
int obtener_tiempo_carga();


#endif
