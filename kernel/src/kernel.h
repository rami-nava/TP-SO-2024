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
extern int corriendo;
extern t_pcb* proceso_en_ejecucion;
extern int ciclo_actual_quantum;
extern bool proceso_en_ejecucion_RR;
extern char* pids;
extern bool cambio_de_proceso;
extern bool instruccion_bloqueante; //Evita casos de bloqueo y fin de quantum al mismo tiempo

extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern int socket_memoria;
extern int servidor_kernel;
extern int socket_cliente_io;
extern char **nombres_recursos;

extern sem_t mutex_pid;
extern sem_t hay_procesos_nuevos;
extern sem_t hay_procesos_ready;
extern sem_t grado_multiprogramacion;
extern sem_t ciclo_actual_quantum_sem;
extern sem_t exit_sem;
extern sem_t rompiendo_reloj;


extern pthread_mutex_t mutex_NEW;
extern pthread_mutex_t mutex_READY;
extern pthread_mutex_t mutex_PROCESOS_DEL_SISTEMA;
extern pthread_mutex_t mutex_AUX_VRR;
extern pthread_mutex_t mutex_corriendo;
extern pthread_cond_t cond_corriendo;
extern pthread_t reloj_RR;
extern pthread_mutex_t proceso_en_ejecucion_RR_mutex;
extern pthread_mutex_t mutex_BLOQUEADOS_recursos;


extern t_list *cola_NEW;
extern t_list *cola_READY;
extern t_list *cola_PROCESOS_DEL_SISTEMA;
extern t_list *cola_AUX_VRR;

extern t_list* interfaces_genericas;
extern t_list* interfaces_stdin;
extern t_list* interfaces_stdout;
extern t_list* interfaces_dialfs;
extern t_list *lista_recursos;

extern pthread_mutex_t mutex_INTERFAZ_GENERICA;
extern pthread_mutex_t mutex_INTERFAZ_STDIN;
extern pthread_mutex_t mutex_INTERFAZ_STDOUT;
extern pthread_mutex_t mutex_INTERFAZ_DIALFS;
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
    sem_t sem_comunicacion_interfaz;
    pthread_mutex_t cola_bloqueado_mutex;
    t_list* cola_bloqueados;
} t_interfaz;    

typedef struct
{
    t_interfaz* interfaz;
    t_paquete* paquete;
    int pid;
} t_paquete_io;  

extern t_interfaz interfaz;
extern arch_config_kernel config_valores_kernel;


//============================================= Inicializacion =====================================================================
void cargar_configuracion(char *);
void manejar_conexion(int);
void inicializar_diccionarios();
void inicializar_colas();
void inicializar_reloj_RR();
void inicializar_planificador();
void inicializar_semaforos();
void crear_colas_bloqueo();
void inicializar_listas();
void agregar_proceso_a_lista_procesos_del_sistema(t_pcb *proceso);
//============================================= Planificador =================================================================================================================
void planificador_largo_plazo();
void planificador_corto_plazo_segun_algoritmo();
void planificador_corto_plazo(t_pcb *(*proximo_a_ejecutar)());
t_pcb *desencolar(t_list *cola);
void encolar(t_list *cola, t_pcb *pcb);
void agregar_PID(void *valor);
void listar_PIDS(t_list *cola);
void cambio_de_estado (t_pcb *pcb, estado_proceso estado_nuevo);
void loggear_cambio_de_estado(int pid, estado_proceso anterior, estado_proceso actual);
void ingresar_a_READY(t_pcb *pcb);
void ingresar_a_NEW(t_pcb *pcb);
void ingresar_a_BLOCKED_IO(t_list* cola, t_pcb *pcb, char* motivo, pthread_mutex_t cola_bloqueado_mutex, char* tipo_interfaz);
void ingresar_a_BLOCKED_recursos(t_pcb *pcb, char* motivo);
void ingresar_de_BLOCKED_a_READY_IO(t_list* cola, pthread_mutex_t mutex_cola_io);
void ingresar_de_BLOCKED_a_READY_recursos(t_pcb* pcb_desbloqueado);
void ingresar_a_AUX_VRR(t_pcb *pcb);
void desalojo(int tipo_interrupcion);
void* comenzar_reloj_RR();
void romper_el_reloj();
void log_ingreso_a_ready();
void log_ingreso_a_aux_vrr();
void logear_cola_io_bloqueados(t_interfaz* interfaz);
void mandar_a_EXIT(t_pcb* proceso, char* motivo);
void sacar_proceso_de_cola_estado_donde_esta(t_pcb* pcb);
bool ocurrio_IO(t_contexto* contexto_ejecucion);

//================================================== PCB =====================================================================================================================
t_pcb *crear_pcb(char*); 
void enviar_pcb_a_cpu(t_pcb *);
t_contexto* enviar_a_cpu(t_pcb* proceso);
void actualizar_PCB(t_pcb* proceso);
void asignar_PBC_a_contexto(t_pcb* proceso);
void liberar_PCB(t_pcb* proceso);
void liberar_recursos_asignados(t_pcb* proceso);
void destruir_PCB(t_pcb* proceso);
void recibir_contexto_actualizado(t_pcb *proceso, t_contexto *contexto_ejecucion);
void loggear_finalizacion_proceso(t_pcb* proceso, char* motivo);
void loggear_motivo_bloqueo(t_pcb* proceso, char* motivo);
void volver_a_CPU(t_pcb* proceso);
bool existe_proceso(int pid);
t_pcb* buscar_pcb_en_lista (t_list* lista, int pid);
t_pcb* buscar_pcb_de_lista_y_eliminar(t_list *lista, int pid_buscado, pthread_mutex_t mutex_cola);
void mostrar_lista_pids(t_list *cola, char *nombre_cola, pthread_mutex_t mutex_cola);

//================================================ Recursos =====================================================================================================================
void liberacion_recursos(t_pcb* );
void liberar_todos_recurso(t_pcb* );
void wait_s(t_pcb *proceso, char **parametros);
void signal_s(t_pcb *proceso, char **parametros);

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
void parse_multiprogramacion(char *linea);
void consola_modificar_multiprogramacion(int nuevo_valor); 
void consola_leer_bitmap(int desde, int hasta);
void parse_leer_bitmap(char *linea);

////======================================== IO ===========================================================================================================
void servidor_kernel_io();
void atender_io(int* socket);
bool peticiones_de_io(t_pcb *proceso, t_interfaz* interfaz);
bool admite_operacion_interfaz(t_interfaz* interfaz, codigo_instrucciones operacion);
t_interfaz* obtener_interfaz_por_nombre(char* nombre_interfaz);
void crear_hilo_io(t_pcb* proceso, t_interfaz* interfaz, t_paquete* peticion);
void crear_hilo_io_generica(t_pcb* proceso, t_interfaz* interfaz, t_paquete* peticion);
bool actualmente_en_IO(t_pcb* pcb);

#endif
