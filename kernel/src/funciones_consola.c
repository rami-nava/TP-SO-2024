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
    listar_PIDS(cola_NEW);
    listar_PIDS(cola_READY);
}

t_pcb* buscar_pcb_de_lista(t_list *lista, int pid_buscado)
{
	int elementos = list_size(lista);
	for (int i = 0; i < elementos; i++)
	{
		t_pcb *pcb = list_get(lista, i);
		if (pid_buscado == pcb->pid)
		{
			list_remove_element(lista, (void*)pcb);
            return pcb;
		}
	}
    return NULL;
}

void consola_finalizar_proceso(int pid) {

  printf("Finalizamos el proceso: %d \n", pid);

  t_pcb* pcb_asociado = NULL;  

  pthread_mutex_lock(&mutex_PROCESOS_DEL_SISTEMA);
  pcb_asociado = buscar_pcb_de_lista(cola_PROCESOS_DEL_SISTEMA,pid);
  pthread_mutex_unlock(&mutex_PROCESOS_DEL_SISTEMA);

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
