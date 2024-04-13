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

#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <commons/memory.h>

#include "socket.h"
#include "contexto.h"
#include "operaciones.h"

extern t_log *io_logger;
extern t_list* interfaces;

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
void inicializar_consola_interactiva();
void iniciar_interfaz(char* nombre, char* path_de_config);
void main_generica(t_interfaz* interfaz_hilo);
void main_stdin(t_interfaz* interfaz_hilo);
void main_stdout(t_interfaz* interfaz_hilo);
void main_dialfs(t_interfaz* interfaz_hilo);
void conectarse_a_kernel(int socket, char* nombre, char* tipo_interfaz);

//.................................. METADATA .....................................................................
void cargamos_cambios_a_metadata_ampliar(int tamanio_nuevo, uint32_t bloque_inicial, char* nombre_archivo, char* path_dial_fs);
void cargamos_cambios_a_metadata_reducir(int tamanio_nuevo, char* nombre_archivo, char* path_dial_fs);
void actualizar_metadata(metadata_archivo* nueva_metadata, char* path_dial_fs);

//..................................FUNCIONES CON ARCHIVOS.....................................................................
void truncar_archivo(char *nombre, int tamanio_nuevo, int socket_kernel);
void ampliar_tamanio_archivo(int tamanio_nuevo, int tamanio_actual_archivo, uint32_t bloque_inicial);
void reducir_tamanio_archivo(int tamanio_nuevo, int tamanio_actual_archivo, uint32_t bloque_inicial);




#endif
