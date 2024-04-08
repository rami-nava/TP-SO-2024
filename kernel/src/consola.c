#include "kernel.h"

void inicializar_consola_interactiva() {
  char* leer_linea;
  while(1) {
    leer_linea = readline(">");  // Lee una línea de la consola
    if (leer_linea) {
      add_history(leer_linea); // Asi podemos usar flechas
      consola_parsear_instruccion(leer_linea);  // Llama a la función para parsear la instrucción
      free(leer_linea);  // Libera la memoria asignada para la línea
    } 
  }
}

void parse_ejecutar_script(char *linea) {
  char **linea_espaciada = string_split(linea, " ");  // Divide la línea en tokens
  char path[256]; 

  if (linea_espaciada && linea_espaciada[1]) {
    if (sscanf(linea_espaciada[1], "\"%255[^\"]\"", path) == 1 ) {
      consola_ejecutar_script(path);
    }
    string_iterate_lines(linea_espaciada, (void*)free); // Libero memoria asignada a cada token
    free(linea_espaciada);  // Libera la memoria asignada al array
  }
}

void parse_iniciar_proceso(char *linea) {
  char path[256]; 
  char **linea_espaciada = string_split(linea," ");  // Divide la línea en tokens
  
  if (linea_espaciada && linea_espaciada[1]) {
    if (sscanf(linea_espaciada[1], "\"%255[^\"]\"", path) == 1 ) {
      consola_iniciar_proceso(path);
    }
    string_iterate_lines(linea_espaciada, (void*)free); // Libero memoria asignada a cada token
    free(linea_espaciada);  // Libera la memoria asignada al array
  }
}

void parse_finalizar_proceso(char *linea) {
  char **linea_espaciada = string_split(linea, " ");  // Divide la línea en tokens
  
  if (linea_espaciada && linea_espaciada[1]) {
    int pid;
    if (sscanf(linea_espaciada[1], "%d", &pid) == 1) {
      consola_finalizar_proceso(pid);
    }
    string_iterate_lines(linea_espaciada, (void*)free); // Libero memoria asignada a cada token
    free(linea_espaciada);  // Libera la memoria asignada al array
  }
}

void parse_detener_planificacion (char* linea) {
  char **linea_espaciada = string_split(linea," ");  // Divide la línea en tokens

  if (linea_espaciada) {
     consola_detener_planificacion();
  }
   string_iterate_lines(linea_espaciada, (void*)free); // Libero memoria asignada a cada token
   free(linea_espaciada);  // Libera la memoria asignada al array
}

void parse_iniciar_planificacion (char* linea) {
  char **linea_espaciada = string_split(linea, " ");  // Divide la línea en tokens

  if (linea_espaciada) {
     consola_iniciar_planificacion();
  }
   string_iterate_lines(linea_espaciada, (void*)free); // Libero memoria asignada a cada token
   free(linea_espaciada);  // Libera la memoria asignada al array
}

void parse_proceso_estado (char* linea) {
  char **linea_espaciada = string_split(linea, " ");  // Divide la línea en tokens

  if (linea_espaciada) {
     printf("Listamos los estados de los procesos\n");
     consola_proceso_estado();
  }
   string_iterate_lines(linea_espaciada, (void*)free); // Libero memoria asignada a cada token
   free(linea_espaciada);  // Libera la memoria asignada al array
}

void consola_parsear_instruccion(char *leer_linea) {
  int comando = -1;

  if (string_contains(leer_linea, "INICIAR_PROCESO")) {
    comando = 0;
  } else if (string_contains(leer_linea, "FINALIZAR_PROCESO")) {
    comando = 1;
  } else if (string_contains(leer_linea, "DETENER_PLANIFICACION")) {
    comando = 2;
  } else if (string_contains(leer_linea, "INICIAR_PLANIFICACION")) {
    comando = 3;
  } else if (string_contains(leer_linea, "EJECUTAR_SCRIPT")) {
    comando = 4;
  } else if (string_contains(leer_linea, "PROCESO_ESTADO")) {
    comando = 5;
  }

  switch (comando) {
    case 0:
      parse_iniciar_proceso(leer_linea);
      break;
    case 1:
      parse_finalizar_proceso(leer_linea);
      break;
    case 2:
      parse_detener_planificacion(leer_linea);
      break;
    case 3:
      parse_iniciar_planificacion(leer_linea);
      break;
    case 4:
      parse_ejecutar_script(leer_linea);
      break;
    case 5:
      parse_proceso_estado(leer_linea);
      break;
    default:
      printf("Comando desconocido: %s\n", leer_linea);
      break;
  }

}