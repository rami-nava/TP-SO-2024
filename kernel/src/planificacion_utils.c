#include "kernel.h"

int quantum;
char* pids; 
static void eliminar_de_cola_io(t_list* lista_interfaces, t_pcb* pcb);

char *estados_procesos[9] = {"NEW", "READY", "EXEC", "BLOCKED_RECURSO", "BLOCKED_IO_GENERICA", "BLOCKED_IO_STDIN", "BLOCKED_IO_STDOUT", "BLOCKED_IO_DIALFS", "SALIDA"};

//==================================================== ENCOLAR Y DESENCOLAR ====================================================================================
void encolar(t_list *cola, t_pcb *pcb){
    list_add(cola, (void *)pcb);
}

t_pcb *desencolar(t_list *cola){
    return (t_pcb *)list_remove(cola, 0);
}

void eliminar_de_cola(t_list *cola, t_pcb *pcb, pthread_mutex_t mutex_cola){
    pthread_mutex_lock(&mutex_cola);
    list_remove_element(cola,pcb);
    pthread_mutex_unlock(&mutex_cola);
}

bool existe_proceso(int pid){
    for(int i = 0; i < list_size(cola_PROCESOS_DEL_SISTEMA); i++){
        t_pcb* proceso_existente = list_get(cola_PROCESOS_DEL_SISTEMA,i);
        if(proceso_existente->pid == pid){
            return true;
        }
    }
    return false;
}
  

t_pcb* buscar_pcb_de_lista_y_eliminar(t_list *lista, int pid_buscado, pthread_mutex_t mutex_lista)
{
  pthread_mutex_lock(&mutex_lista);
  t_pcb* pcb_buscado = buscar_pcb_en_lista(lista, pid_buscado);
  if(pcb_buscado != NULL){
    list_remove_element(lista, (void*)pcb_buscado);
    pthread_mutex_unlock(&mutex_lista);
    return pcb_buscado;
  }else return NULL;
}

t_pcb* buscar_pcb_en_lista (t_list* lista, int pid){
    int elementos = list_size(lista);
	for (int i = 0; i < elementos; i++)
	{
		t_pcb *pcb = list_get(lista, i);
		if (pid == pcb->pid)
		{
            return pcb;
		}
	}
    return NULL;
}

void detener_planificacion() {
    pthread_mutex_lock(&mutex_corriendo);
    while (corriendo == 0) { // Mientras no se detenga
        
        pthread_cond_wait(&cond_corriendo, &mutex_corriendo);
    }
    pthread_mutex_unlock(&mutex_corriendo);
}

void desalojo(int tipo_interrupcion){
    t_paquete *paquete = crear_paquete(DESALOJO);
    pthread_mutex_lock(&proceso_en_ejecucion_RR_mutex);
    proceso_en_ejecucion_RR = false;
    pthread_mutex_unlock(&proceso_en_ejecucion_RR_mutex);
    agregar_entero_a_paquete(paquete,tipo_interrupcion);
    enviar_paquete(paquete, socket_cpu_interrupt);
}

void sacar_proceso_de_cola_estado_donde_esta(t_pcb* pcb){
    //TODO ver como hacer con la IO si finalizan el proceso que esta en IO
    
    estado_proceso estado_actual = pcb->estado;

    switch(estado_actual){
        case NEW:
            eliminar_de_cola(cola_NEW, pcb, mutex_NEW);
            break;
        case READY:
            eliminar_de_cola(cola_READY, pcb, mutex_READY);
            break;
        case BLOCKED_RECURSO:
           //TODO traerlo desde el metodo mandar a exit?
            break;
        case BLOCKED_IO_GENERICA:
            eliminar_de_cola_io(interfaces_genericas, pcb);
            break;
        case BLOCKED_IO_STDIN:
            eliminar_de_cola_io(interfaces_stdin, pcb);
            break;
        case BLOCKED_IO_STDOUT:
            eliminar_de_cola_io(interfaces_stdout, pcb);
            break;
        case BLOCKED_IO_DIALFS:
            eliminar_de_cola_io(interfaces_dialfs, pcb);
            break;
        default:
            break;
    }
}

static void eliminar_de_cola_io(t_list* lista_interfaces, t_pcb* pcb){
    for(int i = 0; i < list_size(lista_interfaces) ; i++){ //Recorre todas las colas de bloqueados de interfaces del tipo en la que se bloqueo
        t_interfaz* interfaz_lista = list_get(lista_interfaces,i);
        t_pcb* proceso_en_io = list_get(interfaz_lista->cola_bloqueados, 0);

        if(proceso_en_io->pid == pcb->pid){ //Si es el proceso que esta actualmente en IO le aviso a IO que deje de procesarlo
            interrumpir_io(interfaz_lista);
        }

        eliminar_de_cola(interfaz_lista->cola_bloqueados,pcb ,interfaz_lista->cola_bloqueado_mutex);
    }
}

bool ocurrio_IO(t_contexto* contexto_ejecucion){
  codigo_instrucciones operacion = contexto_ejecucion->motivo_desalojo->comando;
  
  return operacion == IO_GEN_SLEEP || operacion == IO_STDIN_READ || operacion == IO_STDOUT_WRITE ||
  operacion == IO_FS_CREATE || operacion == IO_FS_DELETE || operacion == IO_FS_TRUNCATE || 
  operacion == IO_FS_WRITE || operacion == IO_FS_READ;
}

//=====================================================LOGS MINIMOS Y OBLIGATORIOS==================================================================================//
void agregar_PID(void *valor){
    t_pcb *pcb = (t_pcb *)valor;
    char *pid = string_itoa(pcb->pid);
    
    string_append_with_format(&pids, " %s ", pid);

    free (pid);
}

void listar_PIDS(t_list *cola) {
    list_iterate(cola, agregar_PID);
}

void mostrar_lista_pids(t_list *cola, char *nombre_cola, pthread_mutex_t mutex_cola)
{
  char *string_pid = NULL;
  char *pids_para_mostrar = NULL;

  pthread_mutex_lock(&mutex_cola);
  int tam_cola = list_size(cola);
  pthread_mutex_unlock(&mutex_cola);

  if (tam_cola == 0) {
    log_info(kernel_logger, "esta vacia la cola %s", nombre_cola);
  }else{ 
    pids_para_mostrar = string_new();
    for (int i = 0; i < tam_cola; i++){
      pthread_mutex_lock(&mutex_cola);

      //Acceso al elemento en el indice i, guardo en pcb local
      t_pcb *pcb = list_get(cola, i);
      string_pid = string_itoa(pcb->pid);

      pthread_mutex_unlock(&mutex_cola);

      // Junto los pids
      string_append(&pids_para_mostrar, string_pid);

      free(string_pid);

      // Separo los PIDs con comas
      if (i < tam_cola - 1) string_append(&pids_para_mostrar, ", ");
    }
      // Mostramos la lista con los pids en la cola 
    log_info(kernel_logger, "Cola %s: [%s]\n", nombre_cola, pids_para_mostrar);
  }

  if (pids_para_mostrar != NULL) {
    free(pids_para_mostrar);  
  }
}

void loggear_cambio_de_estado(int pid, estado_proceso anterior, estado_proceso actual){
    log_info(kernel_logger, "PID: %d - Estado Anterior: %s - Estado Actual: %s", pid, estados_procesos[anterior], estados_procesos[actual]);
}

void log_ingreso_a_ready() 
{
    pthread_mutex_lock(&mutex_READY);
    if(list_size(cola_READY) > 0){
        pids = string_new();
        listar_PIDS(cola_READY);

        log_info(kernel_logger, "Cola Ready %s: [%s] \n", config_valores_kernel.algoritmo, pids);

        free(pids);
    }
    pthread_mutex_unlock(&mutex_READY);
}

void log_ingreso_a_aux_vrr() 
{
    pthread_mutex_lock(&mutex_AUX_VRR);
    if(list_size(cola_AUX_VRR) > 0){
        pids = string_new();
        listar_PIDS(cola_AUX_VRR);

        log_info(kernel_logger, "Cola Auxiliar %s: [%s] \n", config_valores_kernel.algoritmo, pids);

        free(pids);
    }
    pthread_mutex_unlock(&mutex_AUX_VRR);
}

void loggear_motivo_bloqueo(t_pcb* proceso, char* motivo) {
    log_info(kernel_logger,"PID: %d - Bloqueado por: %s\n", proceso->pid, motivo); 
}

void loggear_finalizacion_proceso(t_pcb* proceso, char* motivo) {
    log_info(kernel_logger,"Finaliza el proceso %d - Motivo: %s\n", proceso->pid, motivo); 
}

void logear_cola_io_bloqueados(t_interfaz* interfaz){
  
  pthread_mutex_lock(&interfaz->cola_bloqueado_mutex);
  if(list_size(interfaz->cola_bloqueados) > 0){
        pids = string_new();
        listar_PIDS(interfaz->cola_bloqueados);

        log_info(kernel_logger, "Cola Bloqueados Interfaz %s, %s: [%s] \n", interfaz->tipo_interfaz, interfaz->nombre, pids);

        free(pids);
    }
  pthread_mutex_unlock(&interfaz->cola_bloqueado_mutex);

}
