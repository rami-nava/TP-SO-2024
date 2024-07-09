#include "kernel.h"

pthread_cond_t cond_corriendo;
pthread_mutex_t mutex_corriendo;
pthread_mutex_t mutex_MULTIPROGRAMACION;
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
  } else {
    printf("No estaba pausada la planificacion -_- \n");
  }
}

void consola_modificar_multiprogramacion(int nuevo_valor) 
{
  int valor_actual = config_valores_kernel.grado_multiprogramacion;

	if(nuevo_valor < valor_actual){
		for(int i = nuevo_valor; i < valor_actual; i++){
			sem_wait(&grado_multiprogramacion);
		}
	}
	else{
		for(int i = nuevo_valor; i > valor_actual; i--){
			sem_post(&grado_multiprogramacion);
		}
	}

  pthread_mutex_lock(&mutex_MULTIPROGRAMACION);
  config_valores_kernel.grado_multiprogramacion = nuevo_valor;
  pthread_mutex_unlock(&mutex_MULTIPROGRAMACION);

  printf("Grado de multiprogramacion cambiado a %d\n", nuevo_valor);
}

void consola_leer_bitmap(int desde, int hasta) 
{
  t_interfaz* interfaz = list_get(interfaces_dialfs, 0);

  t_paquete *paquete = crear_paquete(LEER_BITMAP);
  agregar_entero_a_paquete(paquete, desde);
  agregar_entero_a_paquete(paquete, hasta);
  enviar_paquete(paquete, interfaz->socket_conectado);
}

void consola_leer_memoria(int hasta)
{
  t_paquete *paquete = crear_paquete(LEER_MEMORIA);
  agregar_entero_a_paquete(paquete, hasta);
  enviar_paquete(paquete, socket_memoria);
}

void consola_proceso_estado() {
  mostrar_lista_pids(cola_NEW, "NEW", mutex_NEW);
  mostrar_lista_pids(cola_READY, "READY", mutex_READY);

  for(int i = 0 ; i < list_size(interfaces_genericas) ; i++){
    t_interfaz* interfaz = list_get(interfaces_genericas,i);
    char cola[40] = "BLOQUEADOS INTERFAZ ";
    strcat(cola, interfaz->tipo_interfaz);
    strcat(cola, " ");
    strcat(cola, interfaz->nombre);
    mostrar_lista_pids(interfaz->cola_bloqueados, cola, interfaz->cola_bloqueado_mutex);
  }

  for(int i = 0 ; i < list_size(interfaces_stdin) ; i++){
    t_interfaz* interfaz = list_get(interfaces_stdin,i);
    char cola[40] = "BLOQUEADOS INTERFAZ ";
    strcat(cola, interfaz->tipo_interfaz);
    strcat(cola, " ");
    strcat(cola, interfaz->nombre);
    mostrar_lista_pids(interfaz->cola_bloqueados, cola, interfaz->cola_bloqueado_mutex);
  }

  for(int i = 0 ; i < list_size(interfaces_stdout) ; i++){
    t_interfaz* interfaz = list_get(interfaces_stdout,i);
    char cola[40] = "BLOQUEADOS INTERFAZ ";
    strcat(cola, interfaz->tipo_interfaz);
    strcat(cola, " ");
    strcat(cola, interfaz->nombre);
    mostrar_lista_pids(interfaz->cola_bloqueados, cola, interfaz->cola_bloqueado_mutex);
  }

  for(int i = 0 ; i < list_size(interfaces_dialfs) ; i++){
    t_interfaz* interfaz = list_get(interfaces_dialfs,i);
    char cola[40] = "BLOQUEADOS INTERFAZ ";
    strcat(cola, interfaz->tipo_interfaz);
    strcat(cola, " ");
    strcat(cola, interfaz->nombre);
    mostrar_lista_pids(interfaz->cola_bloqueados, cola, interfaz->cola_bloqueado_mutex);
  }

  for(int i = 0 ; i < list_size(lista_recursos) ; i++){
    t_list* lista = list_get(lista_recursos,i);
    char cola[20] = "BLOQUEADOS ";
    strcat(cola, nombres_recursos[i]);
    mostrar_lista_pids(lista, cola, mutex_BLOQUEADOS_recursos);
  }

}

void consola_finalizar_proceso(int pid) {

  printf("Finalizamos el proceso: %d \n", pid);

  t_pcb* pcb_asociado = buscar_pcb_en_lista(cola_PROCESOS_DEL_SISTEMA, pid, mutex_PROCESOS_DEL_SISTEMA);

  //Veo si existe en el sistema
  if(pcb_asociado != NULL){
    //Si existe y esta ejecutando, lo desalojo
    if (pcb_asociado->estado == EXEC){
        desalojo(3);
    }else if(actualmente_en_IO(pcb_asociado)){
      //si esta en IO espero a que vuelva para eliminarlo
      pcb_asociado->eliminado = 1;
    }else{
      //si existe pero esta en otro estado, lo mando a exit
      mandar_a_EXIT(pcb_asociado, "Pedido de finalizacion");
    }
  }else printf("Proceso no encontrado. Intente nuevamente.\n");
}
