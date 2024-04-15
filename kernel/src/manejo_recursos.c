#include "kernel.h"

//============================================ VARIABLES GLOBALES ===========================================
int recursos_existentes;
t_list *lista_recursos;
int *instancias_del_recurso;
int *instancias_maximas_del_recurso;
int recursos_existentes;
char **nombres_recursos;

static int indice_recurso(char *recurso_buscado);
static void eliminar_recurso_proceso(t_list* recursos, char* recurso);
//===========================================================================================================

void crear_colas_bloqueo()
{
    lista_recursos = list_create();

    instancias_del_recurso = NULL;

    //por si alguno hace signal de mas
    instancias_maximas_del_recurso = NULL;

    //aca voy a guardar la cantidad de instancias que tengo para usar [1,2,3]
    char **cantidad_instancias_recursos = config_valores_kernel.instancias_recursos;
    nombres_recursos = config_valores_kernel.recursos;

    recursos_existentes = string_array_size(cantidad_instancias_recursos);

    instancias_del_recurso = malloc(recursos_existentes * sizeof(int));
    instancias_maximas_del_recurso = malloc(recursos_existentes * sizeof(int));

    //por cada recurso del array de recursos que tengo, voy a hacer una cola de bloqueo
    for (int i = 0; i < recursos_existentes; i++)
    {
        /*cantidad_instancias_recursos es un array con las instancias de recursos pero cada cantidad esta
        escrita como un string entonces para poder guardar la cantidad de recursos totales que tengo
        de cada recurso en un int*, necesito pasar cada elemento de cantidad_instancias_recursos a int.*/
        int instancia_en_entero = atoi(cantidad_instancias_recursos[i]);

        //en i=0 el recurso que este en recursos[i] va a tener la cantidad que me marque en cantidad_instancias_recursos[i] :)
        instancias_del_recurso[i] = instancia_en_entero;

        //voy a guardar la cantidad maxima de instancias que tengo porque lo de arriba lo voy a modificar en la ejecucion
        instancias_maximas_del_recurso[i] = instancia_en_entero;

        //voy a crear una lista para cada recurso y la voy a guardar dentro de otra lista
        t_list *cola_bloqueo_del_recurso = list_create();

        //agrego la cola de bloqueo a la lista de recursos, es una lista de listas
        list_add(lista_recursos, cola_bloqueo_del_recurso);
    }

    free_array(cantidad_instancias_recursos);
}

void wait_s(t_pcb *proceso, char **parametros){

    char *recurso = parametros[0];

    int indice_pedido = indice_recurso(recurso);

    //si el recurso no existe, mando el proceso a exit
    if (indice_pedido == -1)
    {
        mandar_a_EXIT(proceso, "Error de wait, el recurso solicitado no existe"); 
        return;
    }

    //restamos la instancia pedida
    //pthread_mutex_lock(&mutex_recursos);
    int instancias = instancias_del_recurso[indice_pedido];
    instancias--;
    instancias_del_recurso[indice_pedido] = instancias;
    //pthread_mutex_unlock(&mutex_recursos);    

    log_info(kernel_logger,"PID: %d - Wait: %s - Instancias: %d\n",proceso->pid, recurso, instancias); 

    if (instancias < 0){
        /*voy a agarrar la cola de bloqueados del recurso que me piden. Como la lista_recursos es una lista
        de punteros a otras colas, lo que voy a hacer es buscar dentro de esa lista, el indice del recurso
        que me pasan por parametro y agarrar la cola del recurso al que nos estamos refiriendo*/
        t_list *cola_bloqueados_recurso = (t_list *)list_get(lista_recursos, indice_pedido);

        //bloqueamos el proceso en la cola de bloqueados del recursos
        list_add(cola_bloqueados_recurso, (void *)proceso);

        ingresar_a_BLOCKED(proceso, recurso);
    }else{
        list_add(proceso->recursos_asignados, (void*)string_duplicate (recurso));
    
        ingresar_a_READY(proceso);
    }
}

void signal_s(t_pcb *proceso, char **parametros)
{
    char *recurso = parametros[0];
    int indice_pedido = indice_recurso(recurso);

     // si el recurso no existe, mando el proceso a exit
    if (indice_pedido == -1)
    {
        //en exit esta el signal y si recibo un recurso invalido, se hace un loop
        mandar_a_EXIT(proceso, "Error de signal, el recurso solicitado no existe"); 
        return;
    }

    // actualizo la cantidad de instancias para el recurso que me pidio el proceso    
    //pthread_mutex_lock(&mutex_recursos);
    int instancias = instancias_del_recurso[indice_pedido];
    instancias++;
    instancias_del_recurso[indice_pedido] = instancias;
    //pthread_mutex_unlock(&mutex_recursos);

    log_info(kernel_logger, "PID: %d - Signal: %s - Instancias: %d\n", proceso->pid, recurso, instancias);

    eliminar_recurso_proceso(proceso->recursos_asignados,recurso); 

    //lo saco de la cola de bloqueados del recurso
    list_remove_element((t_list *)list_get(lista_recursos, indice_pedido), (void *)proceso);

    /*aca vemos que pasa si hay procesos esperando a que ese recurso se libere. Si esta en negativo, es que
    hay un proceso esperando en la cola de bloqueados*/
    if (instancias <= 0)
    {
        t_list *cola_bloqueados_recurso = (t_list *)list_get(lista_recursos, indice_pedido);

        //Nos llega por parametro la cola del recurso y de ahi vamos a sacar nuestro proceso
        if(!list_is_empty(cola_bloqueados_recurso)){
        t_pcb *pcb_desbloqueado = desencolar(cola_bloqueados_recurso);

        list_add(pcb_desbloqueado->recursos_asignados, (void*)string_duplicate (recurso));

        ingresar_a_READY(pcb_desbloqueado);
        }
    }

    //si llega como instruccion algo distinto de EXIT, el proceso sigue su ejecucion 
    if (strncmp (parametros[2], "EXIT", 4)) volver_a_CPU(proceso);
}

static int indice_recurso(char *recurso_buscado)
{
    /*buscamos en el array de recursos que tenemos en la config si existe el recurso que llega por parametro
    y si no existe, devolvemos 1 */
    for (int i = 0; i < recursos_existentes; i++)
    {
        if (recurso_buscado != NULL && !strcmp(recurso_buscado, nombres_recursos[i]))
        {
            return i;
        }
    }
    return -1;
}

static void eliminar_recurso_proceso(t_list* recursos, char* recurso){
    int cant_recursos = list_size(recursos);
    int i;
    
    for(i=0;i<cant_recursos;i++){
        if(!strcmp((char*)list_get(recursos,i), recurso)){
            list_remove(recursos,i);
            return;
        }
    }
}
