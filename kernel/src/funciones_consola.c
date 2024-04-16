#include "kernel.h"

pthread_cond_t cond_corriendo;
pthread_mutex_t mutex_corriendo;
int corriendo = 1;


void consola_ejecutar_script(char* path) 
{
  printf("EJECUTAMOS SCRIPT \n");

  FILE* archivo = fopen(path, "r"); 
  if (archivo == NULL) {
    printf("No se pudo abrir el archivo: %s\n", path);
    return;
  }

  char linea[256]; 

  // Leemos el archivo línea por línea
  while (fgets(linea, sizeof(linea), archivo) != NULL) {
    // Eliminamos el salto de línea al final de la línea
    linea[strcspn(linea, "\n")] = 0;

    consola_parsear_instruccion(linea);
  }

  fclose(archivo);
}


void consola_iniciar_proceso(char *path)
{
    printf("INICIALIZAMOS PROCESO \n");
    crear_pcb(path);
}

void consola_detener_planificacion() {
    printf("PAUSA DE PLANIFICACIÓN \n");

    if(corriendo){
    pthread_mutex_lock(&mutex_corriendo);
    corriendo = 0;  //Bandera en Pausa
    pthread_mutex_unlock(&mutex_corriendo);
    }
    else printf("Ya esta detenida flaco");
}

void consola_iniciar_planificacion() {    
  if(!corriendo) {
       printf("INICIO DE PLANIFICACIÓN \n");
        pthread_mutex_lock(&mutex_corriendo);
        pthread_cond_broadcast(&cond_corriendo);  
        corriendo = 1;  // Bandera sigue
        pthread_mutex_unlock(&mutex_corriendo);
  }
  else {
    printf("No estaba pausada la planificacion -_- \n");
  }
  
}

void consola_proceso_estado() {
  mostrar_lista_pcb(cola_NEW, "NEW", mutex_NEW);
  mostrar_lista_pcb(cola_READY, "READY", mutex_READY);
  mostrar_lista_pcb(cola_BLOCKED, "BLOCKED", mutex_BLOCKED);
}

void mostrar_lista_pcb(t_list *cola, char *nombre_cola, pthread_mutex_t mutex_cola)
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
      // mostramos la lista con los pids en la cola 
    log_info(kernel_logger, "Cola %s: [%s]\n", nombre_cola, pids_para_mostrar);
  }

  if (pids_para_mostrar != NULL) {
    free(pids_para_mostrar);  // Free pids if it was allocated
  }
}

//funcion sicario, te busca y te mata
t_pcb* buscar_pcb_de_lista_y_eliminar(t_list *lista, int pid_buscado, pthread_mutex_t mutex_lista)
{
  t_pcb* pcb_buscado = buscar_pcb_en_lista(lista, pid_buscado);

  pthread_mutex_lock(&mutex_lista);
  if(pcb_buscado != NULL){
    list_remove_element(lista, (void*)pcb_buscado);
    pthread_mutex_unlock(&mutex_lista);
    return pcb_buscado;
  }else return NULL;
}

void consola_finalizar_proceso(int pid) {

  printf("Finalizamos el proceso: %d \n", pid);

  t_pcb* pcb_asociado = NULL;  

  //saco el proceso de los procesos del sistema
  pcb_asociado = buscar_pcb_de_lista_y_eliminar(cola_PROCESOS_DEL_SISTEMA,pid, mutex_PROCESOS_DEL_SISTEMA);

  //veo si existe en el sistema
  if(pcb_asociado != NULL){
    //si existe y esta ejecutando, lo desalojo
    if (pcb_asociado->estado == EXEC){
        t_paquete *paquete_fin = crear_paquete(DESALOJO);
        int interrupcion_exit = 3;
        agregar_entero_a_paquete(paquete_fin, interrupcion_exit);
        enviar_paquete(paquete_fin, socket_cpu_interrupt);
    }else{
      //si existe pero esta en otro estado, lo mando a exit
      mandar_a_EXIT(pcb_asociado, "Pedido de finalizacion");
    }
  }else printf("Proceso no encontrado. Intente nuevamente.\n");
}
