#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include <commons/temporal.h>

#include "socket.h"
#include "operaciones.h"
#include "contexto.h"


//================================================== Variables =====================================================================
extern t_log *kernel_logger;
extern t_config *config;
extern char* pids;
extern int corriendo;
extern int quantum;
extern bool proceso_en_ejecucion_RR;

extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern int socket_memoria;
extern int servidor_kernel;
extern int socket_cliente_io;

extern sem_t mutex_pid;
extern sem_t hay_procesos_nuevos;
extern sem_t hay_procesos_ready;
extern sem_t grado_multiprogramacion;


extern pthread_mutex_t mutex_NEW;
extern pthread_mutex_t mutex_READY;
extern pthread_mutex_t mutex_exec;
extern pthread_mutex_t mutex_exit;
extern pthread_mutex_t mutex_blocked;
extern pthread_mutex_t mutex_recursos;
extern pthread_mutex_t mutex_colas;
extern pthread_mutex_t mutex_corriendo;
extern pthread_cond_t cond_corriendo;
extern pthread_t reloj_RR;
extern pthread_mutex_t proceso_en_ejecucion_RR_mutex;


extern t_list *cola_NEW;
extern t_list *cola_READY;
extern t_list *cola_BLOCKED;

//extern t_list *cola_BLOCKED;
//extern t_list *cola_EXEC;
//extern t_list *cola_EXIT;

extern t_list* interfaces_genericas;
extern t_list* interfaces_stdin;
extern t_list* interfaces_stdout;
extern t_list* interfaces_dialfs;


//==============================================================================================================================

typedef struct
{
    char *ip_escucha;
    char *puerto_escucha;
    char *ip_memoria;
    char *puerto_memoria;
    char *ip_cpu;
    char *puerto_cpu_dispatch;
    char *puerto_cpu_interrupt;
    char *algoritmo;
    int quantum;
    char **recursos;
    char **instancias_recursos;
    int grado_multiprogramacion;
} arch_config_kernel;

typedef struct
{
    int socket_conectado;
    char* nombre;
    char* tipo_interfaz;
} t_interfaz;    

extern t_interfaz interfaz;
extern arch_config_kernel config_valores_kernel;


//============================================= Inicializacion =====================================================================
void cargar_configuracion(char *);
void manejar_conexion(int);
void iniciar_proceso(char *, int, int);
void inicializar_diccionarios();
void inicializar_colas();
void inicializar_reloj_RR();
void inicializar_planificador();
void inicializar_semaforos();
void crear_colas_bloqueo();
void inicializar_listas();
//============================================= Planificador =================================================================================================================
void planificador_largo_plazo();
void planificador_corto_plazo_segun_algoritmo();
void planificador_corto_plazo(t_pcb *(*proximo_a_ejecutar)());
t_pcb *desencolar(t_list *cola);
void encolar(t_list *cola, t_pcb *pcb);
void agregar_PID(void *valor);
void listar_PIDS(t_list *cola);
void cambio_de_estado (t_pcb *pcb, estado_proceso estado_nuevo);
void ingresar_a_READY(t_pcb *pcb);
void ingresar_a_NEW(t_pcb *pcb);
void desalojo(int tipo_interrupcion);
void* comenzar_reloj_RR();
void log_ingreso_a_ready();
void mandar_a_EXIT(t_pcb* proceso, char* motivo);

////======================================== MEMORIA ===========================================================================================================
void enviar_pcb_a_memoria(t_pcb *, int, op_code);
op_code esperar_respuesta_memoria(int);

//================================================== PCB =====================================================================================================================
t_pcb *crear_pcb(char*); 
void enviar_pcb_a_cpu(t_pcb *);
t_contexto* enviar_a_cpu(t_pcb* proceso);
void actualizar_PCB(t_pcb* proceso);
void asignar_PBC_a_contexto(t_pcb* proceso);
void liberar_PCB(t_pcb* proceso);
void destruir_PCB(t_pcb* proceso);
void recibir_contexto_actualizado(t_pcb *proceso, t_contexto *contexto_ejecucion);
void loggear_finalizacion_proceso(t_pcb* proceso, char* motivo);
void volver_a_CPU(t_pcb* proceso);

//================================================ Recursos =====================================================================================================================
int indice_recurso (char* );
void asignacion_recursos(t_pcb* );
char* recibir_peticion_recurso(t_pcb* );
void liberacion_recursos(t_pcb* );
bool proceso_reteniendo_recurso(t_pcb* ,char* );
void liberar_todos_recurso(t_pcb* );

//================================================ Consola ==================================================================================================================
void inicializar_consola_interactiva();
void parse_iniciar_proceso(char *linea);
void parse_finalizar_proceso(char *linea);
void parse_detener_planificacion (char* linea);
void parse_iniciar_planificacion (char* linea);
void parse_ejecutar_script(char *linea);
void parse_proceso_estado (char* linea);
void consola_parsear_instruccion(char *leer_linea);
void consola_finalizar_proceso(int pid);
void consola_detener_planificacion();
void consola_iniciar_planificacion();
void consola_proceso_estado();
void detener_planificacion();
void consola_ejecutar_script(char *path);
void consola_iniciar_proceso(char *path);
void detener_planificacion();


////======================================== IO ===========================================================================================================
void servidor_kernel_io();
void atender_io(int socket);
bool peticiones_de_io(t_pcb *proceso, t_interfaz* interfaz);
bool admite_operacion_interfaz(t_interfaz* interfaz, codigo_instrucciones operacion);
t_interfaz* obtener_interfaz_por_nombre(char* nombre_interfaz);

#endif
