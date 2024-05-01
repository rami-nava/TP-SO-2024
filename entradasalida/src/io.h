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

#include "socket.h"
#include "contexto.h"
#include "operaciones.h"

extern t_list* interfaces;

extern t_list* bloques_iniciales;
extern t_log* dialfs_logger;
extern char* path_dial_fs;
extern FILE* archivo_de_bloques;
extern int tamanio_bloque;
extern int cantidad_bloques;
extern int tamanio_archivo_bloques;
extern int retraso_compactacion;

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

//.................................. INICIALIZACION IO .......................................................................
void iniciar_interfaz(char* nombre, char* path_de_config);
void main_generica(t_interfaz* interfaz_hilo);
void main_stdin(t_interfaz* interfaz_hilo);
void main_stdout(t_interfaz* interfaz_hilo);
void main_dialfs(t_interfaz* interfaz_hilo);
void conectarse_a_kernel(int socket, op_code codigo_operacion ,char* nombre, char* tipo_interfaz);
void handle_sigint(int sig);
void desconectarse();
void desconectar_memoria_stdin();
void desconectar_memoria_stdout();
void desconectar_memoria_dialfs();

//.................................. METADATA .....................................................................
void cargamos_cambios_a_metadata_ampliar(int tamanio_nuevo, uint32_t bloque_inicial, char* nombre_archivo);
void cargamos_cambios_a_metadata_reducir(int tamanio_nuevo, char* nombre_archivo);
metadata_archivo* levantar_metadata(char* nombre_archivo);

//.................................. ARCHIVOS .......................................................................
FILE* levantar_archivo_bloque();
void crear_archivo_de_bloque();
void cargar_bitmap();
void leer_bitmap();
void agregar_bloques(uint32_t cantidad_bloques_a_agregar, uint32_t bloque_inicial);
uint32_t buscar_bloque_libre(uint32_t bloque_inicial);
void eliminar_bloques(uint32_t cantidad_bloques_a_eliminar, uint32_t bloque_inicial);
uint32_t buscar_bloque_inicial_libre();
void compactar(uint32_t cantidad_bloques_a_compactar, uint32_t bloque_final_archivo);
void marcar_bloque_ocupado(int index);
bool esta_libre(int index);
bool bloques_contiguos(uint32_t cantidad_bloques_a_buscar, uint32_t bloque_final_archivo);

#endif
