#include "kernel.h"

pthread_cond_t cond_corriendo;
pthread_mutex_t mutex_corriendo;
int corriendo = 1;

void consola_ejecutar_script(char *path)
{
    printf("EJECUTAMOS SCRIPT \n");
    FILE* script = fopen(path, "r");
        if (script == NULL) {
            printf("Error opening file\n");
            return;
        }

    char linea[256];
    while (fgets(linea, sizeof(linea), script) != NULL) {
    consola_parsear_instruccion(linea);
    }

    fclose(script);
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

void consola_finalizar_proceso(int pid) {

    printf("Finalizamos proceso el proceso %d \n", pid);

    t_pcb* pcb_asociado = NULL;  
    int estado = -1;    
    /*
    // Recorremos cola de Blocked
    pthread_mutex_lock(&mutex_blocked);
    if (list_size(cola_BLOCKED) > 0) {
        for (int i = 0; i < list_size(cola_BLOCKED); i++) {
            t_pcb* pcb = list_get(cola_BLOCKED, i);
            if (pcb->pid == pid) {
                pcb_asociado = pcb;
                estado = BLOCKED;
                break;
            }
        }
    }
    pthread_mutex_unlock(&mutex_blocked);

    // Recorremos cola de Execute
    if (pcb_asociado == NULL) {
        pthread_mutex_lock(&mutex_exec);
        if (list_size(cola_EXEC) > 0) {
            for (int i = 0; i < list_size(cola_EXEC); i++) {
                t_pcb* pcb = list_get(cola_EXEC, i);
                if (pcb->pid == pid) {
                    pcb_asociado = pcb;
                    estado = EXEC;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutex_exec);
    }*/

   // Recorremos cola de NEW
    if (pcb_asociado == NULL) {
        pthread_mutex_lock(&mutex_NEW);
        if (list_size(cola_NEW) > 0) {
            for (int i = 0; i < list_size(cola_NEW); i++) {
                t_pcb* pcb = list_get(cola_NEW, i);
                if (pcb->pid == pid) {
                    pcb_asociado = pcb;
                    estado = NEW;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutex_NEW);
    }

    // Recorremos cola de READY
    if (pcb_asociado == NULL) {
        pthread_mutex_lock(&mutex_READY);
        if (list_size(cola_READY) > 0) {
            for (int i = 0; i < list_size(cola_READY); i++) {
                t_pcb* pcb = list_get(cola_READY, i);
                if (pcb->pid == pid) {
                    pcb_asociado = pcb;
                    estado = READY;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mutex_READY);
    }

    //Si encuentra el pcb
    if (pcb_asociado != NULL) {

        //Lo desalojo
        if(estado == EXEC) {
        t_paquete *paquete_fin = crear_paquete(FINALIZAR_PROCESO);
        int interrupcion_exit = 2;
        agregar_entero_a_paquete(paquete_fin, interrupcion_exit);
        enviar_paquete(paquete_fin, socket_cpu_interrupt);
        eliminar_paquete(paquete_fin);

        liberar_PCB(pcb_asociado);
        } 
        else {
        //El proceso entra en EXIT
        cambio_de_estado (pcb_asociado, SALIDA); 

        //Avisas pq finalizo el proceso
        loggear_finalizacion_proceso(pcb_asociado, "SUCCESS"); 

        //Liberamos memoria
        liberar_PCB(pcb_asociado);

        sem_post(&grado_multiprogramacion); 
        }
    } else printf("Proceso no encontrado. Intente nuevamente.\n");
    
}