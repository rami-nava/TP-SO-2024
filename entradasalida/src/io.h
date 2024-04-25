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

#include "socket.h"
#include "contexto.h"
#include "operaciones.h"

extern t_list* interfaces;
extern t_log* dialfs_logger;

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


extern FILE* archivo_de_bloques;
extern int tam_bloque;

//.................................. INICIALIZACION IO .......................................................................
void iniciar_interfaz(char* nombre, char* path_de_config);
void main_generica(t_interfaz* interfaz_hilo);
void main_stdin(t_interfaz* interfaz_hilo);
void main_stdout(t_interfaz* interfaz_hilo);
void main_dialfs(t_interfaz* interfaz_hilo);
void conectarse_a_kernel(int socket, op_code codigo_operacion ,char* nombre, char* tipo_interfaz);
void handle_sigint(int sig);
void desconectarse();

//.................................. METADATA .....................................................................
void cargamos_cambios_a_metadata_ampliar(int tamanio_nuevo, uint32_t bloque_inicial, char* nombre_archivo, char* path_dial_fs);
void cargamos_cambios_a_metadata_reducir(int tamanio_nuevo, char* nombre_archivo, char* path_dial_fs);
metadata_archivo* levantar_metadata(char* nombre_archivo, char* path_dial_fs);

//.................................. ARCHIVOS .......................................................................
FILE* levantar_archivo_bloque(char* path_archivo_bloques);
void crear_archivo_de_bloque(char* path_archivo_bloques, int tamanio_archivo_bloques);
void cargar_bitmap(int cantidad_bloques, char* path_bitmap);
void liberar_un_bloque_del_fs();
void agregar_bloque(uint32_t bloque_a_agregar);
void eliminar_bloque(uint32_t bloque_a_eliminar);
uint32_t buscar_bloque_libre();
void limpiar_posiciones(t_bitarray* un_espacio, int posicion_inicial, int tamanio_proceso);
void ocupar_un_bloque_del_fs();
void compactar();
void marcar_bloque_ocupado(int index);
bool bloques_contiguos(uint32_t cantidad_bloques_a_buscar);
uint32_t buscar_bloque_en_fs(uint32_t bloque_a_leer, uint32_t bloque_inicial);



#endif
