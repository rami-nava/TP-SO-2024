#include "operaciones.h"

//===================================================== PAQUETES =============================================================================

t_paquete* crear_paquete(op_code codigo)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
	return paquete;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
  // Calculamos el tamanio del buffer + 2 ints (codigo + tamanio)
  int bytes = paquete->buffer->size + 2 * sizeof(int);

  // Serializamos el paquete a un buffer
  void* buffer = serializar_paquete(paquete, bytes);

  // Envia el buffer
  send(socket_cliente, buffer, bytes, 0);

  free(buffer);

  eliminar_paquete(paquete);
}


void* serializar_paquete(t_paquete* paquete, int bytes)
{
  int desplazamiento = 0;

  void* buffer_serializado = malloc(bytes);

  // Copio el código de operación al inicio del buffer
  memcpy(buffer_serializado + desplazamiento, &(paquete->codigo_operacion), sizeof(int));

  // Incremento desplazamiento para apuntar después del código
  desplazamiento += sizeof(int);

  // Copio el tamaño del buffer original al buffer final
  memcpy(buffer_serializado + desplazamiento, &(paquete->buffer->size), sizeof(int));

  // Incrementa desplazamiento después del tamaño
  desplazamiento += sizeof(int);

  // Copio los bytes del buffer original al buffer final
  memcpy(buffer_serializado + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

  // Incrementa desplazamiento por los bytes copiados
  desplazamiento+= paquete->buffer->size;

  return buffer_serializado;
}

void agregar_entero_a_paquete(t_paquete* paquete, int entero) {

  // Re-aloco el buffer del paquete para hacer espacio para un entero más
  paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(int));

  // Copio el entero al final del buffer
  memcpy(paquete->buffer->stream + paquete->buffer->size, &entero, sizeof(int));

  // Incremento el tamaño del buffer
  paquete->buffer->size += sizeof(int);

}


void agregar_entero_sin_signo_a_paquete(t_paquete* paquete, uint32_t entero_no_signado)
{
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(uint32_t));
    memcpy(paquete->buffer->stream + paquete->buffer->size, &entero_no_signado, sizeof(uint32_t));
    paquete->buffer->size += sizeof(uint32_t);
}

void agregar_byte_sin_signo_a_paquete(t_paquete* paquete, uint8_t byte_no_signado)
{
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(uint8_t));
    memcpy(paquete->buffer->stream + paquete->buffer->size, &byte_no_signado, sizeof(uint8_t));
    paquete->buffer->size += sizeof(uint8_t);
}

// Función para agregar datos de tamaño variable a un paquete
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio) {

  // Re-aloca el buffer del paquete para hacer espacio para los nuevos datos y su tamanio
  paquete->buffer->stream = realloc(paquete->buffer->stream,
  paquete->buffer->size + tamanio + sizeof(int));

  // Copia el tamaño de los datos al final del buffer actual
  memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));

  // Copia los datos después del tamaño
  memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

  // Incrementa el tamaño total del buffer
  paquete->buffer->size += tamanio + sizeof(int);

}

void agregar_bytes_a_paquete(t_paquete* paquete, void* contenido, uint32_t tamanio_bytes)
{
    int size = paquete->buffer->size;

    // Re-aloca el buffer del paquete para hacer espacio para los nuevos datos y su tamanio
    paquete->buffer->stream = realloc(paquete->buffer->stream, 
    size + tamanio_bytes + sizeof(uint32_t));

    // Copiar el tamaño de los bytes al stream
    memcpy(paquete->buffer->stream + size, &tamanio_bytes, sizeof(uint32_t));

    // Copiar los bytes al stream
    memcpy(paquete->buffer->stream + size + sizeof(uint32_t), contenido, tamanio_bytes);
    
    // Incrementa el tamaño total del buffer
    paquete->buffer->size += tamanio_bytes + sizeof(uint32_t);
}

void agregar_cadena_a_paquete(t_paquete* paquete, char* palabra)
{
	agregar_a_paquete(paquete, (void*)palabra, string_length(palabra) +1);
}

void agregar_array_cadenas_a_paquete(t_paquete* paquete, char** palabras)
{
	int cant_elementos = string_array_size(palabras);
	agregar_entero_a_paquete(paquete,cant_elementos);

	for(int i=0; i<cant_elementos; i++)
	{
		agregar_cadena_a_paquete(paquete, palabras[i]);
	}
}

void agregar_lista_de_cadenas_a_paquete(t_paquete* paquete, t_list* palabras)
{
	int cant_elementos = list_size(palabras);
	agregar_entero_a_paquete(paquete,cant_elementos);

	for(int i=0; i<cant_elementos; i++)
	{
		char* palabra = list_get(palabras, i);
		agregar_cadena_a_paquete(paquete, palabra);
	}
}

void agregar_lista_de_accesos_a_paquete(t_paquete* paquete, t_list* accesos)
{
	int tam_lista = list_size(accesos);
	agregar_entero_a_paquete(paquete,tam_lista);

	for(int i = 0; i < tam_lista; i++)
	{
        t_acceso_memoria* acceso = list_get(accesos, i);
        agregar_entero_sin_signo_a_paquete(paquete, acceso->direccion_fisica);
        agregar_entero_sin_signo_a_paquete(paquete, acceso->tamanio);
    }
}
	

void agregar_puntero_a_paquete(t_paquete* paquete, void* puntero, uint32_t tamanio_puntero)
{
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + sizeof(uint32_t) + tamanio_puntero);

    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio_puntero, sizeof(uint32_t));
    paquete->buffer->size += sizeof(uint32_t);

    if (tamanio_puntero > 0) {
        memcpy(paquete->buffer->stream + paquete->buffer->size, puntero, tamanio_puntero);
        paquete->buffer->size += tamanio_puntero;
    }
}


t_paquete* recibir_paquete(int conexion)  
{
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    // Recibo el código de operación
    if(recv(conexion, &(paquete->codigo_operacion),sizeof(paquete->codigo_operacion),MSG_WAITALL)==-1)
    {
        // Si hay error, devolvemos un paquete con código de operación -1 (ERROR)
        paquete->codigo_operacion = -1;
        return paquete;
    }

    // Recibo el tamaño del buffer
    recv(conexion, &(paquete->buffer->size),sizeof(paquete->buffer->size),MSG_WAITALL);
    
    // Reservo memoria para el stream según el tamaño
    paquete->buffer->stream = malloc(paquete->buffer->size);
    
    // Recibo los datos en el stream
    recv(conexion, paquete->buffer->stream, paquete->buffer->size,MSG_WAITALL);  
    
    return paquete;
}

void* recibir_buffer(int socket_cliente){

    // Recibir el tamanio
    int tamanio = 0;
    int *size = &tamanio; 
	recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL);

    // Recibir el buffer
	void* buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

int recibir_operacion(int socket_cliente){
	int cod_op = 0;

    // Si recibo algo lo devuelvo, si no, devuelvo -1
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

char* sacar_cadena_de_paquete(void** stream) {

  int tamanio_cadena = -1;
  char* cadena = NULL;

  // Copio el tamaño de la cadena desde el stream
  memcpy(&tamanio_cadena, *stream, sizeof(int));

  // Avanzo el puntero del stream más allá del tamanio
  *stream += sizeof(int);

  // Aloco memoria para la cadena
  cadena = malloc(tamanio_cadena);

  // Copio los bytes de la cadena desde el stream
  memcpy(cadena, *stream, tamanio_cadena);

  // Avanzo el puntero del stream hasta el final de la cadena
  *stream += tamanio_cadena;

  return cadena;
}

void* sacar_bytes_de_paquete(void** stream)
{
    uint32_t tamanio_bytes = 0;
    void* contenido = NULL;

    // Copio el tamaño de los bytes desde el stream
    memcpy(&tamanio_bytes, *stream, sizeof(int)); 

    //Avanzo el puntero del stream hasta el final de los bytes
    *stream += sizeof(int); 

    // Aloco memoria para los bytes
    contenido = malloc(tamanio_bytes);

    // Copio los bytes desde el stream
    memcpy(contenido, *stream, tamanio_bytes);

    // Avanzo el puntero del stream hasta el final de los bytes
    *stream += tamanio_bytes;

    return contenido;
}

int sacar_entero_de_paquete(void** stream)
{
	int entero = -1;

	//Copio los bytes del numero desde el stream
	memcpy(&entero, *stream, sizeof(int));

    // Avanzo el puntero del stream hasta el final del entero
	*stream += sizeof(int);

	return entero;
}

uint32_t sacar_entero_sin_signo_de_paquete(void** stream)
{
    uint32_t numero = 0;

    //Copio los bytes del numero desde el stream
    memcpy(&numero, *stream, sizeof(uint32_t));

    // Avanzo el puntero del stream hasta el final del numero
    *stream += sizeof(uint32_t);

    return numero;
}

uint8_t sacar_byte_sin_signo_de_paquete(void** stream) 
{
    uint8_t byte = 0;

    memcpy(&byte, *stream, sizeof(uint8_t));
    *stream += sizeof(uint8_t);

    return byte;
}

char** sacar_array_cadenas_de_paquete(void** stream)
{
	char** varias_palabras =  string_array_new();

	int cant_elementos = sacar_entero_de_paquete(stream);

	for(int i=0; i<cant_elementos; i++)
	{
		string_array_push(&varias_palabras, sacar_cadena_de_paquete(stream));
	}

	return varias_palabras;
}

t_list* sacar_lista_de_cadenas_de_paquete(void** stream) 
{
	t_list* varias_palabras =  list_create();

	int cant_elementos = sacar_entero_de_paquete(stream);

	for(int i=0; i<cant_elementos; i++)
	{
		char* cadena = sacar_cadena_de_paquete(stream);
		list_add(varias_palabras, cadena);
	}

	return varias_palabras;
}


t_list* sacar_lista_de_accesos_de_paquete(void** stream) 
{
	t_list* accesos =  list_create();

	int cant_elementos = sacar_entero_de_paquete(stream);

	for(int i = 0; i< cant_elementos; i++)
    {
        t_acceso_memoria* acceso_memoria = malloc(sizeof(t_acceso_memoria));
        acceso_memoria->direccion_fisica = sacar_entero_sin_signo_de_paquete(stream);
        acceso_memoria->tamanio = sacar_entero_sin_signo_de_paquete(stream);
        list_add(accesos, acceso_memoria);
    }

	return accesos;
}


void* sacar_puntero_de_paquete(void** stream)
{
    void* puntero = NULL;
    uint32_t tamanio_puntero = 0;

    memcpy(&tamanio_puntero, *stream, sizeof(uint32_t));
    *stream += sizeof(uint32_t);

    if (tamanio_puntero > 0) {
        puntero = malloc(tamanio_puntero);
        memcpy(puntero, *stream, tamanio_puntero);
        *stream += tamanio_puntero;
    }

    return puntero;
}

//============================================== Liberar memoria ===================================================
void free_array(char ** array){
	int tamanio = string_array_size(array);
	for (int i = 0; i<tamanio; i++) free(array[i]);
	free(array);
}


void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}




