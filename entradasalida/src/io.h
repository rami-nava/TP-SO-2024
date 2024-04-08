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

#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h>
#include <commons/memory.h>

#include "socket.h"
#include "contexto.h"
#include "operaciones.h"




extern t_config *config;
extern t_log *io_logger;
extern int socket_memoria;
extern int socket_kernel;

extern t_list *bloques_reservados;
extern t_list* procesos_en_filesystem;

extern int tamanio_archivo_bloques;
extern t_dictionary* diccionario_archivos_abiertos;
extern int proximo_bloque_inicial;


//STRUCTS//
typedef struct  
 {
    char* ip_memoria;
    char* puerto_memoria;
    char* ip_kernel;
    char* puerto_kernel;
    char* path_base_dialfs;
    int block_size;
    int block_count;
    int tiempo_unidad_de_trabajo;
    char* tipo_interfaz;
    
} arch_config;

typedef struct 
{
    char* nombre_archivo; 
    int tamanio_archivo; 
    int bloque_inicial;
} metadata_archivo;

typedef struct 
{
    int pid;
} t_proceso_en_io;



extern arch_config config_valores_io;
extern FILE* archivo_de_bloques;
extern char* path_dial_fs;
extern int tam_bloque;

//..................................FUNCIONES UTILES IO.....................................................................
void protocolo_multihilo();
void cargar_configuracion(char*);
void cargamos_cambios_a_fcb_ampliar(int tamanio_nuevo, uint32_t bloque_inicial, char* nombre_archivo);
void cargamos_cambios_a_fcb_reducir(int tamanio_nuevo, char* nombre_archivo);
void actualizar_metadata(metadata_archivo* nueva_metadata);
t_proceso_en_io* buscar_proceso_en_io(int pid);

//..................................FUNCIONES ARCHIVOS.....................................................................

void truncar_archivo(char *nombre, int tamanio_nuevo, int socket_kernel);

void ampliar_tamanio_archivo(int tamanio_nuevo, int tamanio_actual_archivo, uint32_t bloque_inicial);

void reducir_tamanio_archivo(int tamanio_nuevo, int tamanio_actual_archivo, uint32_t bloque_inicial);



//..................................FUNCIONES ARCHIVOS DE BLOQUE.....................................................................
void leer_archivo(char *nombre_archivo, uint32_t puntero_archivo, uint32_t direccion_fisica);
void escribir_archivo(char* nombre_archivo, uint32_t puntero_archivo, void* contenido);
void escribir_contenido_en_archivo(uint32_t bloque_a_escribir, uint32_t bloque_inicial, void* contenido, uint32_t puntero, char* nombre_archivo);
void solicitar_informacion_memoria(uint32_t direccion_fisica, int tam_bloque, char* nombre_archivo, uint32_t puntero_archivo);
void crear_archivo_de_bloque();
void cerrar_archivo(char* nombre_archivo);
void mapear_archivo_de_bloques();


#endif
