#include "io.h"

static void consola_parsear_instruccion(char *leer_linea);
static void parse_iniciar_interfaz(char *linea);

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

static void consola_parsear_instruccion(char *leer_linea) {
  int comando = -1;

  if (string_contains(leer_linea, "INICIAR_INTERFAZ")) {
    comando = 0;
  }
  switch (comando) {
    case 0:
      parse_iniciar_interfaz(leer_linea);
      break;
    default:
      printf("Comando desconocido: %s\n", leer_linea);
      break;
  }
}
static void parse_iniciar_interfaz(char *linea) {
  char **linea_espaciada = string_split(linea, " ");  // Divide la línea en tokens 
  char* nombre = malloc(sizeof(linea_espaciada[1])); //Error de memoria => CORREGIR
  char path[256]; 
  
  if (linea_espaciada[0] && linea_espaciada[1] && linea_espaciada[2]) {
    if (sscanf(linea_espaciada[2], "\"%255[^\"]\"", path) == 1) {
      strcpy(nombre,linea_espaciada[1]); //Falta hacer el free(nombre)!!
      iniciar_interfaz(nombre,path);
    }
    string_iterate_lines(linea_espaciada, (void*)free); // Libero memoria asignada a cada token
    free(linea_espaciada);  // Libera la memoria asignada al array
  }
}
