#include "kernel.h"

// EJECUTAR SCRIPT //
void ejecutar_script(char *path)
{
    printf("EJECUTAMOS SCRIPT \n");
}

void consola_detener_planificacion() {
    printf("PAUSA DE PLANIFICACIÓN \n");

    if(corriendo_pf){
    pthread_mutex_lock(&mutex_corriendo_pf);
    corriendo_pf = 0;  //Bandera en Pausa
    pthread_mutex_unlock(&mutex_corriendo_pf);
    pf_listo = 0;
    }

    if(corriendo){
    pthread_mutex_lock(&mutex_corriendo);
    corriendo = 0;  //Bandera en Pausa
    pthread_mutex_unlock(&mutex_corriendo);
    pf_listo = 0;
    }
    else printf("Ya esta detenida flaco");
}

void consola_iniciar_planificacion() {

    if(!corriendo_pf){
        pthread_mutex_lock(&mutex_corriendo_pf);
        pthread_cond_broadcast(&cond_corriendo_pf);  
        corriendo_pf = 1;  // Bandera sigue
        pthread_mutex_unlock(&mutex_corriendo_pf);
    }
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

void consola_finalizar_proceso(int pid) {

    printf("Finalizamos proceso el proceso %d \n", pid);

    t_pcb* pcb_asociado = NULL;  
    int estado = -1;  

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
    }

   // Recorremos cola de NEW
    if (pcb_asociado == NULL) {
        pthread_mutex_lock(&mutex_new);
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
        pthread_mutex_unlock(&mutex_new);
    }

    // Recorremos cola de READY
    if (pcb_asociado == NULL) {
        pthread_mutex_lock(&mutex_ready);
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
        pthread_mutex_unlock(&mutex_ready);
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

        eliminar_pcb(pcb_asociado);
        } 
        else {
        // Remuevo el pcb del diccionario
        pthread_mutex_lock(&mutex_colas);
        list_remove_element(dictionary_int_get(diccionario_colas, estado), pcb_asociado);
        pthread_mutex_unlock(&mutex_colas);

        // Lo meto en Exit
        pthread_mutex_lock(&mutex_exit);
        meter_en_cola(pcb_asociado, EXIT, cola_EXIT);
        pthread_mutex_unlock(&mutex_exit);

        // sacamos el proceso de la lista de exit
        pthread_mutex_lock(&mutex_exit);
        list_remove_element(dictionary_int_get(diccionario_colas, EXIT), pcb_asociado);
        pthread_mutex_unlock(&mutex_exit);

        // le mandamos esto a memoria para que destruya las estructuras
        enviar_pcb_a_memoria(pcb_asociado, socket_memoria, FINALIZAR_EN_MEMORIA);
        printf("Enviando a memoria liberar estructuras del proceso \n");

        int fin_ok = 0;
        recv(socket_memoria, &fin_ok, sizeof(int), 0);

        if (fin_ok != 1)
        {
        printf("No se pudieron eliminar estructuras en memoria del proceso PID[%d]\n", pcb_asociado->pid);
        }
    
        log_info(kernel_logger, "Finaliza el proceso %d - Motivo: SUCCESS\n", pcb_asociado->pid);

        eliminar_pcb(pcb_asociado);
        sem_post(&grado_multiprogramacion);
    }
    } else {
        printf("Proceso no encontrado. Intente nuevamente.\n");
    }
}

// PROCESO_ESTADO //
void consola_proceso_estado() {
    listar_PIDS(cola_NEW);
    listar_PIDS(cola_READY);
}
