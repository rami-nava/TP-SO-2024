#ifndef IO_H_
#define IO_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <math.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <commons/memory.h>
#include <commons/temporal.h>
#include <commons/collections/dictionary.h>

#include "socket.h"
#include "contexto.h"
#include "operaciones.h"

extern t_list* interfaces;

extern t_list* bloques_iniciales;
extern t_dictionary* nombre_con_bloque_inicial;
extern t_log* dialfs_logger;
extern char* path_dial_fs;
extern FILE* archivo_de_bloques;
extern int tamanio_bloque;
extern int cantidad_bloques;
extern int tamanio_archivo_bloques;
extern int retraso_compactacion;
extern bool compactar_desde_comienzo;

//STRUCTS//
typedef struct 
{
    char* nombre_archivo; 
    int tamanio_archivo; 
    int bloque_inicial;
} metadata_archivo;

typedef struct 
{
    char* nombre_interfaz;
    t_config* config_interfaz; 
} t_interfaz;

typedef struct 
{
    int pid;
    int tiempo_sleep;
} t_peticion_generica;

typedef struct 
{
    int pid;
    t_list* direcciones_fisicas;
    uint32_t tamanio_registro;
} t_peticion_std;

typedef struct
{
    int op_code;
    int pid;
    char* nombre;
    int desde;
    int hasta;
    uint32_t puntero_archivo;
    uint32_t tamanio;
    t_list* direcciones_fisicas;
    uint32_t tamanio_archivo;
} t_peticion_dialfs;

//.................................. INICIALIZACION IO .......................................................................
void iniciar_interfaz(char* nombre, char* path_de_config);
void main_generica(t_interfaz* interfaz_hilo);
void main_stdin(t_interfaz* interfaz_hilo);
void main_stdout(t_interfaz* interfaz_hilo);
void main_dialfs(t_interfaz* interfaz_hilo);
void conectarse_a_kernel(int socket, op_code codigo_operacion ,char* nombre, char* tipo_interfaz);

//.................................. DESCONEXION IO .......................................................................
void handle_sigint(int sig);
void desconectarse();
void desconectar_generica();
void desconectar_stdin();
void desconectar_stdout();
void desconectar_dialfs();
void desconectar_memoria_stdin();
void desconectar_memoria_stdout();
void desconectar_memoria_dialfs();

//.................................. METADATA .....................................................................
void cargamos_cambios_a_metadata_ampliar(int tamanio_nuevo, char* nombre_archivo);
void cargamos_cambios_a_metadata_reducir(int tamanio_nuevo, char* nombre_archivo);
metadata_archivo* levantar_metadata(char* nombre_archivo);
void modificar_metadata_bloque_inicial(uint32_t nuevo_bloque_inicial, uint32_t bloque_inicial);

//.................................. ARCHIVOS .......................................................................
FILE* levantar_archivo_bloque();
void crear_archivo_de_bloque();
void cargar_bitmap();
void leer_bitmap(int desde, int hasta);
void agregar_bloques(uint32_t cantidad_bloques_a_agregar, uint32_t bloque_inicial);
void eliminar_bloques(uint32_t cantidad_bloques_a_eliminar, uint32_t bloque_inicial);
uint32_t buscar_bloque_inicial_libre();
void compactar(uint32_t cantidad_bloques_a_compactar, uint32_t bloque_final_archivo);
int compactar_desde_el_comienzo(uint32_t bloque_final_archivo);
bool bloques_contiguos(uint32_t cantidad_bloques_a_buscar, uint32_t bloque_final_archivo);

//.................................. MEMORIA .......................................................................


#endif
