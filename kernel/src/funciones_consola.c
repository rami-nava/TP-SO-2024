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
  mostrar_lista_pids(cola_NEW, "NEW", mutex_NEW);
  mostrar_lista_pids(cola_READY, "READY", mutex_READY);
  //Ver como mostrar los bloqueados TODO
}

void consola_finalizar_proceso(int pid) {

  printf("Finalizamos el proceso: %d \n", pid);

  t_pcb* pcb_asociado = NULL;  

  //Saco el proceso de los procesos del sistema
  pcb_asociado = buscar_pcb_de_lista_y_eliminar(cola_PROCESOS_DEL_SISTEMA,pid, mutex_PROCESOS_DEL_SISTEMA);

  //Veo si existe en el sistema
  if(pcb_asociado != NULL){
    //Si existe y esta ejecutando, lo desalojo
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
