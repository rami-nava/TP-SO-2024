#include "kernel.h"

/*
void atender_clientes_io()
{
    int* socket_cliente_io = malloc(sizeof(int));
	*socket_cliente_io = esperar_cliente(servidor_kernel);

    if(*socket_cliente_io != -1){ 
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void *)atender_io, &socket_cliente_io);
        pthread_detach(hilo_io);
    }else {
        log_error(kernel_logger, "Error al escuchar clientes... Finalizando servidor \n"); 
    }

}

void atender_io(void* socket_cliente_io)
{
    int socket_io = *(int*)socket_cliente_io;
    while(1)
    {
        t_paquete* paquete = recibir_paquete(*socket_io);
        void* stream = paquete->buffer->stream;
        switch(paquete->codigo_operacion)
        {
            case INTERFAZ_GENERICA:
            char* nombre = sacar_cadena_de_paquete(&stream);
            break;
            case INTERFAZ_STDIN:
            char* nombre = sacar_cadena_de_paquete(&stream);
            break;
            case INTERFAZ_STDOUT:
            char* nombre = sacar_cadena_de_paquete(&stream);
            break;
            case INTERFAZ_DIALFS:
            char* nombre = sacar_cadena_de_paquete(&stream);
            break;
            default:
            break;
        }
        eliminar_paquete(paquete);
    }

}*/