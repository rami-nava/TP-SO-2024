#include "kernel.h"

//============================================ VARIABLES GLOBALES ===========================================
int recursos_existentes;
t_list *lista_recursos;
int *instancias_del_recurso;
char **nombres_recursos;
char **cantidad_instancias_recursos;
pthread_mutex_t mutex_BLOQUEADOS_recursos;

static int indice_recurso(char *recurso_buscado);
static void eliminar_recurso_de_proceso(t_list* recursos, char* recurso);
static t_list* obtener_lista_recurso_buscado(int indice);
static void eliminar_proceso_colas_bloqueo(t_pcb* proceso);
//===========================================================================================================

void crear_colas_bloqueo(){
    instancias_del_recurso = NULL;
    cantidad_instancias_recursos = config_valores_kernel.instancias_recursos;
    nombres_recursos = config_valores_kernel.recursos;

    //Obtengo la cantidad de recursos que tengo
    recursos_existentes = string_array_size(cantidad_instancias_recursos);

    instancias_del_recurso = malloc(recursos_existentes * sizeof(int));

    //Por cada recurso que tengo debo crear una lista de bloqueos
    for (int i = 0; i < recursos_existentes; i++)
    {
        int instancia_en_entero = atoi(cantidad_instancias_recursos[i]);
        
        //Guardo las instancias totales 
        instancias_del_recurso[i] = instancia_en_entero;

        //Creo una lista para cada recurso
        t_list *cola_bloqueo_del_recurso = list_create();

        //Lo agrego a la lista de recursos
        list_add(lista_recursos, cola_bloqueo_del_recurso);
    }

    free_array(cantidad_instancias_recursos);
}

void wait_s(t_pcb *proceso, char **parametros){
    char *recurso = parametros[0];
    int indice_pedido = indice_recurso(recurso);

    //Si el recurso no existe, mando el proceso a exit
    if (indice_pedido == -1)
    {
        mandar_a_EXIT(proceso, "Error de wait, el recurso solicitado no existe"); 
        return;
    }

    // Actualizo la cantidad de instancias para el recurso que me pidio el proceso
    int instancias = instancias_del_recurso[indice_pedido];
    instancias--;
    instancias_del_recurso[indice_pedido] = instancias;

    log_info(kernel_logger,"PID: %d - Wait: %s - Instancias: %d\n",proceso->pid, recurso, instancias); 

    //Si no hay instancias disponibles
    if (instancias < 0)
    {
        //Busco el recurso en la lista de bloqueados y agrego el proceso a la cola de bloqueos del recurso
        list_add(obtener_lista_recurso_buscado(indice_pedido), (void *)proceso);

        ingresar_a_BLOCKED_recursos(proceso, recurso);
    } else //Si hay instancias disponibles
    {
        //Lo agrego a la lista de recursos asignados del proceso
        char* recurso_para_asignar = string_duplicate(recurso);
        list_add(proceso->recursos_asignados, recurso_para_asignar);

        ingresar_a_READY(proceso);
    }
}

void signal_s(t_pcb *proceso, char **parametros){
    char *recurso_pedido = parametros[0];
    int indice_pedido = indice_recurso(recurso_pedido);

     // Si el recurso no existe, mando el proceso a exit
    if (indice_pedido == -1){
        mandar_a_EXIT(proceso, "Error de signal, el recurso solicitado no existe"); 
        return;
    }

    // Actualizo la cantidad de instancias para el recurso que me pidio el proceso
    int instancias = instancias_del_recurso[indice_pedido];
    instancias++;
    instancias_del_recurso[indice_pedido] = instancias;

    log_info(kernel_logger, "PID: %d - Signal: %s - Instancias: %d\n", proceso->pid, recurso_pedido, instancias);

    char* recurso_para_asignar = string_duplicate(recurso_pedido);

    eliminar_recurso_de_proceso(proceso->recursos_asignados,recurso_pedido); 

    t_list *cola_bloqueados_recurso = obtener_lista_recurso_buscado(indice_pedido);

    //Saco al proceso de la cola de bloqueados del recurso
    list_remove_element(cola_bloqueados_recurso, (void *)proceso);

    //Hay procesos esperando a que ese recurso se libere?
    if (instancias <= 0){
        
        //Hay procesos bloqueados esperando ese recurso
        if(!list_is_empty(cola_bloqueados_recurso)){

        //Saco el primer proceso de la lista de bloqueados
        t_pcb *pcb_desbloqueado = desencolar(cola_bloqueados_recurso);

        //Lo agrego a la lista de recursos asignados del proceso
        //list_add(pcb_desbloqueado->recursos_asignados, (void*)string_duplicate (recurso_pedido));

        list_add(pcb_desbloqueado->recursos_asignados, recurso_para_asignar);
        
        //Lo mando a READY
        ingresar_de_BLOCKED_a_READY_recursos(pcb_desbloqueado);
        }
    }else free(recurso_para_asignar);

    //si llega como instruccion algo distinto de EXIT, el proceso sigue su ejecucion 
    if (strncmp (parametros[2], "EXIT", 4)) volver_a_CPU(proceso);
    else{
        eliminar_proceso_colas_bloqueo(proceso);
    }
}

/*Si el proceso hizo un wait pero nunca le di el recurso, tengo que igualmente sacarlo de la cola
de bloqueados del recurso y sumarle una instancia disponible al recurso.*/
static void eliminar_proceso_colas_bloqueo(t_pcb* proceso){

    //Busco en todas las colas de bloqueo de recursos si esta el proceso y si esta lo elimino
    for(int i = 0 ; i < list_size(lista_recursos); i++){
        if(buscar_pcb_en_lista(obtener_lista_recurso_buscado(i), proceso->pid) != NULL){
            pthread_mutex_lock(&mutex_BLOQUEADOS_recursos);
            list_remove_element(obtener_lista_recurso_buscado(i), (void*)proceso);
            pthread_mutex_unlock(&mutex_BLOQUEADOS_recursos);
            char* nombre_recurso_pedido = nombres_recursos[i];
            int indice_pedido = indice_recurso(nombre_recurso_pedido);

            int instancias = instancias_del_recurso[indice_pedido];
            instancias++;
            instancias_del_recurso[indice_pedido] = instancias;
        }
    }
}

static t_list* buscar_proceso_colas_bloqueo(t_pcb* proceso){
    t_list* recursos_pedidos = list_create();

     for(int i = 0 ; i < list_size(lista_recursos); i++){
        char* recurso = nombres_recursos[i];
        if(buscar_pcb_en_lista(obtener_lista_recurso_buscado(i), proceso->pid) != NULL){
            list_add(recursos_pedidos, recurso);
        }
     }
    return recursos_pedidos;
}

static t_list* obtener_lista_recurso_buscado(int indice){
    t_list *lista_buscada = (t_list *)list_get(lista_recursos, indice);
    return lista_buscada;
}

static int indice_recurso(char *recurso_buscado)
{
    //Buscamos en la lista de recursos si existe el recurso que llega por parametro
    for (int i = 0; i < recursos_existentes; i++)
    {
        if (recurso_buscado != NULL && !strcmp(recurso_buscado, nombres_recursos[i]))
        {
            return i;
        }
    }
    return -1;
}

static void eliminar_recurso_de_proceso(t_list* recursos, char* recurso){
    int cant_recursos = list_size(recursos);
    
    for(int i = 0; i < cant_recursos; i++){
        if(!strcmp((char*)list_get(recursos,i), recurso)){
            list_remove_and_destroy_element(recursos,i, free);
            return;
        }
    }
}

void liberar_recursos_asignados(t_pcb* proceso) {
    int cant_recursos = list_size(proceso->recursos_asignados);

    if (cant_recursos != 0) {
        t_list* recursos_a_liberar = list_duplicate(proceso->recursos_asignados);

        for (int i = 0; i < cant_recursos; i++) {
            char* recurso = (char*)list_get(recursos_a_liberar, i);
            char* parametros[3] = {recurso, "", "EXIT"};
            signal_s(proceso, parametros);
        }

        list_destroy(recursos_a_liberar);
    }

    //De esta manera, si lo finalizan, hace signal en todos los recursos donde hizo wait
    t_list* recursos_a_signal = buscar_proceso_colas_bloqueo(proceso);

    if(!list_is_empty(recursos_a_signal)){
        for (int i = 0; i < list_size(recursos_a_signal); i++) {
            char* recurso = (char*)list_get(recursos_a_signal, i);
            char* parametros[3] = {recurso, "", "EXIT"};
            signal_s(proceso, parametros);
        }
    }
    list_destroy(recursos_a_signal);
}