#include "kernel.h"

static void agregar_interfaz(op_code tipo, void* stream, int socket_cliente_io); 

void atender_io() 
{
    while(1) {
        t_paquete* paquete = recibir_paquete(socket_cliente_io);
        void* stream = paquete->buffer->stream;

        agregar_interfaz(paquete->codigo_operacion, stream, socket_cliente_io); 
        eliminar_paquete(paquete);
    }
}

static void agregar_interfaz(op_code tipo, void* stream, int socket_cliente_io) 
{
    t_interfaz* interfaz_nueva = malloc(sizeof(t_interfaz));
    interfaz_nueva->nombre = sacar_cadena_de_paquete(&stream);
    interfaz_nueva->tipo_interfaz = sacar_cadena_de_paquete(&stream);
    interfaz_nueva->socket_conectado = socket_cliente_io;

    switch(tipo) {
        case INTERFAZ_GENERICA:
            list_add(interfaces_genericas, interfaz_nueva);
            log_info(kernel_logger, "Se agrego interfaz generica: %s \n", interfaz_nueva->nombre);
            break;
        case INTERFAZ_STDIN:
            list_add(interfaces_stdin, interfaz_nueva);
            break;
        case INTERFAZ_STDOUT:
            list_add(interfaces_stdout, interfaz_nueva);
            break;
        case INTERFAZ_DIALFS:
            list_add(interfaces_dialfs, interfaz_nueva);
            break;
        default:
            break;
    }
}

bool peticiones_de_io(t_pcb *proceso, t_interfaz* interfaz) 
{
    if (interfaz == NULL) {
        log_error(kernel_logger, "Interfaz inexistente");
        mandar_a_EXIT(proceso, "ERROR");
        return false;
    }

    if(!admite_operacion_interfaz(interfaz, contexto_ejecucion->motivo_desalojo->comando))
    {
        log_error(kernel_logger, "Interfaz incorrecta");
        mandar_a_EXIT(proceso, "ERROR");
        return false;
    }

    return true;
}

t_interfaz* obtener_interfaz_por_nombre(char* nombre_interfaz) {

    // INTERFACES GENERICAS
    for (int i = 0; i < list_size(interfaces_genericas); i++) {
    t_interfaz* interfaz_nueva = list_get(interfaces_genericas, i);
    if (strcmp(nombre_interfaz, interfaz_nueva->nombre) == 0) {
        return interfaz_nueva;
            }   
        }
    
    // INTERFACES STDIN
    for (int i = 0; i < list_size(interfaces_stdin); i++) {
    t_interfaz* interfaz_nueva = list_get(interfaces_stdin, i);
    if (strcmp(nombre_interfaz, interfaz_nueva->nombre) == 0) {
        return interfaz_nueva;
            }
        }

    // INTERFACES STDOUT
    for (int i = 0; i < list_size(interfaces_stdout); i++) {
    t_interfaz* interfaz_nueva = list_get(interfaces_stdout, i);
    if (strcmp(nombre_interfaz, interfaz_nueva->nombre) == 0) {
        return interfaz_nueva;
            }
        }

    // INTERFACES DIALFS;
    for (int i = 0; i < list_size(interfaces_dialfs); i++) {
    t_interfaz* interfaz_nueva = list_get(interfaces_dialfs, i);
    if (strcmp(nombre_interfaz, interfaz_nueva->nombre) == 0) {
        return interfaz_nueva;
            }
        }
    
    return NULL;
}

bool admite_operacion_interfaz(t_interfaz* interfaz, codigo_instrucciones operacion) {

    if (strcmp(interfaz->tipo_interfaz, "GENERICA") == 0) {
        return (operacion == IO_GEN_SLEEP);
    } 
    if (strcmp(interfaz->tipo_interfaz, "STDIN") == 0) {
        return (operacion == IO_STDIN_READ);
    } 
    if (strcmp(interfaz->tipo_interfaz, "STDOUT") == 0) {
        return (operacion == IO_STDOUT_WRITE);
    } 
    if (strcmp(interfaz->tipo_interfaz, "DIALFS") == 0) {
        return (operacion == IO_FS_CREATE) || (operacion == IO_FS_DELETE) || (operacion == IO_FS_TRUNCATE) || (operacion == IO_FS_WRITE) || (operacion == IO_FS_READ);
    }
    return false;
}

 /*if(paquete->codigo_operacion == INTERFAZ_GENERICA) {

        t_interfaz* interfaz_nueva = malloc(sizeof(t_interfaz));
        interfaz_nueva->nombre = sacar_cadena_de_paquete(&stream);
        interfaz_nueva->socket_conectado = socket_cliente_io;

        list_add(interfaces_genericas, interfaz_nueva);
        log_info(kernel_logger, "Se agrego interfaz generica: %s \n", interfaz_nueva->nombre);
        } else {
            perror("TROLEEEAMOSSS\n");
            abort();
        }*/
//static void atender_io(int socket_cliente_io);
/*
void atender_clientes_io()
{
	int socket_cliente_io = esperar_cliente(servidor_kernel);

    if(socket_cliente_io != -1){ 
        pthread_t hilo_io;
        pthread_create(&hilo_io, NULL, (void *)atender_io, &socket_cliente_io);
        pthread_detach(hilo_io);
    }else {
        log_error(kernel_logger, "Error al escuchar clientes... Finalizando servidor \n"); 
    }
}
*/