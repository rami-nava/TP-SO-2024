#include "io.h"

metadata_archivo* levantar_metadata (char* nombre) {

    char* path = string_from_format ("%s/%s", path_dial_fs, nombre);

    t_config* archivo = config_create (path);

    metadata_archivo* metadata = malloc (sizeof (metadata_archivo)); 
    metadata->nombre_archivo = string_duplicate(config_get_string_value (archivo, "NOMBRE_ARCHIVO"));
    metadata->bloque_inicial = config_get_int_value(archivo, "BLOQUE_INICIAL");
    metadata->tamanio_archivo = config_get_int_value (archivo, "TAMANIO_ARCHIVO");

	config_destroy (archivo);
    free(path);
    return metadata;
}

void cargamos_cambios_a_metadata_ampliar(int tamanio_nuevo, char* nombre_archivo) 
{
	char* puntero_auxiliar = NULL;

	char* path = string_from_format ("%s/%s", path_dial_fs, nombre_archivo);
    t_config * archivo = config_create (path);

	puntero_auxiliar = string_from_format("%d", tamanio_nuevo);
    config_set_value(archivo, "TAMANIO_ARCHIVO", puntero_auxiliar);
	free(puntero_auxiliar);

    config_save_in_file(archivo, path);

    free(path);
    config_destroy(archivo);
}

void modificar_metadata_bloque_inicial(uint32_t nuevo_bloque_inicial, uint32_t bloque_inicial) {
    char* nombre_archivo = obtener_nombre_de_archivo(bloque_inicial, nuevo_bloque_inicial);

	char* path = string_from_format ("%s/%s", path_dial_fs, nombre_archivo);
    t_config * archivo = config_create (path);

    char* puntero_auxiliar = string_from_format("%d", nuevo_bloque_inicial);
    config_set_value(archivo, "BLOQUE_INICIAL", puntero_auxiliar);
    free(puntero_auxiliar);
    config_save_in_file(archivo, path);

    free(path);
    free(nombre_archivo);
    config_destroy(archivo);
}

void cargamos_cambios_a_metadata_reducir(int tamanio_nuevo, char* nombre_archivo) {

	char* puntero_auxiliar = NULL;

    char* path = string_from_format ("%s/%s", path_dial_fs, nombre_archivo);
    t_config * archivo = config_create (path);
	
	puntero_auxiliar = string_from_format("%d", tamanio_nuevo);
    config_set_value(archivo, "TAMANIO_ARCHIVO", puntero_auxiliar);
	free(puntero_auxiliar);

    config_save_in_file(archivo, path);

    free(path);
    config_destroy(archivo);
}
