#include "kernel.h"

pthread_mutex_t mutex_INTERFAZ_GENERICA;
pthread_mutex_t mutex_INTERFAZ_STDIN;
pthread_mutex_t mutex_INTERFAZ_STDOUT;
pthread_mutex_t mutex_INTERFAZ_DIALFS;

static void esperar_io_generica(t_interfaz* interfaz);
static void esperar_io(t_interfaz* interfaz);

void servidor_kernel_io(){
    
     servidor_kernel = iniciar_servidor(config_valores_kernel.ip_escucha, config_valores_kernel.puerto_escucha);

     while(1){
          int* socket_cliente_io = malloc(sizeof(int));
          *socket_cliente_io = esperar_cliente(servidor_kernel);
          
          if(*socket_cliente_io != -1)
          {
               pthread_t hilo_io;
               pthread_create(&hilo_io, NULL, (void* ) atender_io, socket_cliente_io);
               pthread_detach(hilo_io);
          }
     }
}


void atender_io(int* socket_io) 
{
    int socket_cliente_io = *(int*)socket_io;
    while(1) {
        t_paquete* paquete = recibir_paquete(socket_cliente_io);
        void* stream = paquete->buffer->stream;

        t_interfaz* interfaz_nueva = malloc(sizeof(t_interfaz));
        interfaz_nueva->nombre = sacar_cadena_de_paquete(&stream);
        interfaz_nueva->tipo_interfaz = sacar_cadena_de_paquete(&stream);
        interfaz_nueva->socket_conectado = socket_cliente_io;
        sem_init(&interfaz_nueva->sem_comunicacion_interfaz, 0, 1);
        pthread_mutex_init(&interfaz_nueva->cola_bloqueado_mutex, NULL);
        interfaz_nueva->cola_bloqueados = list_create();

    switch(paquete->codigo_operacion) {
        case INTERFAZ_GENERICA:
            pthread_mutex_lock(&mutex_INTERFAZ_GENERICA);
            interfaz_nueva->tiempo_sleep = sacar_entero_de_paquete(&stream);
            list_add(interfaces_genericas, interfaz_nueva);
            pthread_mutex_unlock(&mutex_INTERFAZ_GENERICA);
            log_info(kernel_logger, "Se agrego interfaz GENERICA: %s \n", interfaz_nueva->nombre);
            break;
        case INTERFAZ_STDIN:
            pthread_mutex_lock(&mutex_INTERFAZ_STDIN);
            list_add(interfaces_stdin, interfaz_nueva);
            pthread_mutex_unlock(&mutex_INTERFAZ_STDIN);
            log_info(kernel_logger, "Se agrego interfaz STDIN: %s \n", interfaz_nueva->nombre);
            break;
        case INTERFAZ_STDOUT:
            pthread_mutex_lock(&mutex_INTERFAZ_STDOUT);
            list_add(interfaces_stdout, interfaz_nueva);
            pthread_mutex_unlock(&mutex_INTERFAZ_STDOUT);
            log_info(kernel_logger, "Se agrego interfaz STDOUT: %s \n", interfaz_nueva->nombre);
            break;
        case INTERFAZ_DIALFS:
            pthread_mutex_lock(&mutex_INTERFAZ_DIALFS);
            list_add(interfaces_dialfs, interfaz_nueva);
            pthread_mutex_unlock(&mutex_INTERFAZ_DIALFS);
            log_info(kernel_logger, "Se agrego interfaz DIALFS: %s \n", interfaz_nueva->nombre);
            break;
        default:
            break;
        }
        eliminar_paquete(paquete);
    }
}

bool peticiones_de_io(t_pcb *proceso, t_interfaz* interfaz) 
{
    //Si existe la interfaz y esta conectada
    if (interfaz == NULL) {
        log_error(kernel_logger, "Interfaz inexistente");
        mandar_a_EXIT(proceso, "ERROR con IO");
        return false;
    }

    //Si admite el comando de instruccion
    if(!admite_operacion_interfaz(interfaz, contexto_ejecucion->motivo_desalojo->comando))
    {
        log_error(kernel_logger, "Interfaz incorrecta");
        mandar_a_EXIT(proceso, "ERROR con IO");
        return false;
    }

    return true;
}

t_interfaz* obtener_interfaz_por_nombre(char* nombre_interfaz) {

    pthread_mutex_lock(&mutex_INTERFAZ_GENERICA);
    int tamanio_lista_generica = list_size(interfaces_genericas);
    pthread_mutex_unlock(&mutex_INTERFAZ_GENERICA);

    // INTERFACES GENERICAS
    for (int i = 0; i < tamanio_lista_generica; i++) {
    pthread_mutex_lock(&mutex_INTERFAZ_GENERICA);
    t_interfaz* interfaz_nueva = list_get(interfaces_genericas, i);
    pthread_mutex_unlock(&mutex_INTERFAZ_GENERICA);
    
    if (strcmp(nombre_interfaz, interfaz_nueva->nombre) == 0) {
        return interfaz_nueva;
            }   
        }
    
    // INTERFACES STDIN
    pthread_mutex_lock(&mutex_INTERFAZ_STDIN);
    int tamanio_lista_stdin = list_size(interfaces_stdin);
    pthread_mutex_unlock(&mutex_INTERFAZ_STDIN);

    for (int i = 0; i < tamanio_lista_stdin; i++) {
    pthread_mutex_lock(&mutex_INTERFAZ_STDIN);
    t_interfaz* interfaz_nueva = list_get(interfaces_stdin, i);
    pthread_mutex_unlock(&mutex_INTERFAZ_STDIN);

    if (strcmp(nombre_interfaz, interfaz_nueva->nombre) == 0) {
        return interfaz_nueva;
            }
        }

    // INTERFACES STDOUT
    pthread_mutex_lock(&mutex_INTERFAZ_STDOUT);
    int tamanio_lista_stdout = list_size(interfaces_stdout);
    pthread_mutex_unlock(&mutex_INTERFAZ_STDOUT);

    for (int i = 0; i < tamanio_lista_stdout; i++) {
    pthread_mutex_lock(&mutex_INTERFAZ_STDOUT);
    t_interfaz* interfaz_nueva = list_get(interfaces_stdout, i);
    pthread_mutex_unlock(&mutex_INTERFAZ_STDOUT);

    if (strcmp(nombre_interfaz, interfaz_nueva->nombre) == 0) {
        return interfaz_nueva;
            }
        }

    // INTERFACES DIALFS
    pthread_mutex_lock(&mutex_INTERFAZ_DIALFS);
    int tamanio_lista_dialfs = list_size(interfaces_dialfs);
    pthread_mutex_unlock(&mutex_INTERFAZ_DIALFS);

    for (int i = 0; i < tamanio_lista_dialfs; i++) {
    pthread_mutex_lock(&mutex_INTERFAZ_DIALFS);
    t_interfaz* interfaz_nueva = list_get(interfaces_dialfs, i);
    pthread_mutex_unlock(&mutex_INTERFAZ_DIALFS);

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

void crear_hilo_io(t_pcb* proceso, t_interfaz* interfaz, t_paquete* peticion) {
    char motivo[35] = "";

    strcat(motivo, "INTERFAZ ");
    strcat(motivo, interfaz->tipo_interfaz);
    strcat(motivo, ": ");
    strcat(motivo, interfaz->nombre);

    if(!strcmp(config_valores_kernel.algoritmo, "VRR")){
        sem_wait(&ciclo_actual_quantum_sem);
        proceso->quantum = ciclo_actual_quantum;
    }
    
    ingresar_a_BLOCKED_IO(interfaz->cola_bloqueados ,proceso, motivo, interfaz->cola_bloqueado_mutex);
    logear_cola_io_bloqueados(interfaz); //NO es obligatorio

    sem_wait(&interfaz->sem_comunicacion_interfaz);
    enviar_paquete(peticion, interfaz->socket_conectado);

    pthread_t hilo_manejo_io;
    pthread_create(&hilo_manejo_io, NULL, (void* ) esperar_io, interfaz);
    pthread_detach(hilo_manejo_io);

    sem_post(&interfaz->sem_comunicacion_interfaz);

}

static void esperar_io(t_interfaz* interfaz)
{
int termino_io;

recv(interfaz->socket_conectado, &termino_io, sizeof(int), MSG_WAITALL);

ingresar_de_BLOCKED_a_READY_IO(interfaz->cola_bloqueados, interfaz->cola_bloqueado_mutex);
}

void crear_hilo_io_generica(t_pcb* proceso, t_interfaz* interfaz, t_paquete* peticion) {
    
    char motivo[35] = "";

    strcat(motivo, "INTERFAZ ");
    strcat(motivo, interfaz->tipo_interfaz);
    strcat(motivo, ": ");
    strcat(motivo, interfaz->nombre);

    if(!strcmp(config_valores_kernel.algoritmo, "VRR")){
        sem_wait(&ciclo_actual_quantum_sem);
        proceso->quantum = ciclo_actual_quantum;
    }

    ingresar_a_BLOCKED_IO(interfaz->cola_bloqueados ,proceso, motivo, interfaz->cola_bloqueado_mutex);
    logear_cola_io_bloqueados(interfaz); //NO es obligatorio

    //Esperamos a que la interfaz se desbloquee
    sem_wait(&interfaz->sem_comunicacion_interfaz);

    enviar_paquete(peticion, interfaz->socket_conectado);

    pthread_t hilo_manejo_io;
    pthread_create(&hilo_manejo_io, NULL, (void* ) esperar_io_generica, interfaz);
    pthread_detach(hilo_manejo_io);

    sem_post(&interfaz->sem_comunicacion_interfaz);

}

static void esperar_io_generica(t_interfaz* interfaz)
{
    int tiempo_sleep_total = interfaz->tiempo_sleep * interfaz->tiempo_sleep_kernel * 1000;

    usleep(tiempo_sleep_total);

    ingresar_de_BLOCKED_a_READY_IO(interfaz->cola_bloqueados, interfaz->cola_bloqueado_mutex);
}