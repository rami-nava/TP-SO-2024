#include "memoria.h"

void* espacio_usuario;
//t_list* procesos_en_memoria; creo que no va esto
t_list* marcos;

//============================================ ESPACIO DE USUARIO ===========================================================
void creacion_espacio_usuario(){
	espacio_usuario = malloc(config_valores_memoria.tam_memoria);

	if (espacio_usuario == NULL) {
        perror ("No se pudo alocar memoria al espacio de usuario.");
        abort();
    } 

	//Se inicializa con 0
	memset(espacio_usuario,0,config_valores_memoria.tam_memoria); 

	marcos = list_create();

	crear_marcos_memoria();
}

void acceso_a_espacio_usuario(int pid, char* accion, uint32_t dir_fisica, uint32_t tamanio)
{
	sleep(config_valores_memoria.retardo_respuesta / 1000);
	
	log_info(memoria_logger, "ACCESO A ESPACIO USUARIO - PID %d - ACCION: %s - DIRECCION FISICA: %d - TAMAÑO: %d", pid, accion, dir_fisica, tamanio); 
}

//============================================ CREACION DE PROCESOS && MARCOS DE MEMORIA ====================================================

void crear_estructuras_memoria(int pid, FILE* archivo){

	t_proceso_en_memoria* proceso = malloc(sizeof(t_proceso_en_memoria));
	proceso->pid = pid;
	proceso->instrucciones = list_create();
    proceso->paginas_en_memoria = list_create();

	leer_instrucciones_desde_archivo(proceso, archivo);

	list_add(procesos_en_memoria, proceso);
}

void crear_marcos_memoria() {

	for(int i = 0; i < cantidad_marcos; i++) {
		t_marco* marco = malloc(sizeof(t_marco)); 

		marco->nro_pagina = -1;
		marco->nro_marco = i;
		marco->libre = 1;
		marco->pid_proceso = -1;
        
		list_add(marcos, marco);
	}
}

void finalizar_en_memoria(int pid){
	//TODO NUEVO 1C2024
	//Obtengo el proceso por el pid
	t_proceso_en_memoria *proceso = obtener_proceso_en_memoria(pid);

	//Libero las paginas
	int cantidad_paginas_de_proceso = list_size(proceso->paginas_en_memoria);
	quitar_marcos_a_proceso(proceso, cantidad_paginas_de_proceso);

	//Elimino el proceso de la lista
	list_remove_element(procesos_en_memoria, proceso);

	//Limpio su lista de instrucciones
	list_destroy(proceso->instrucciones);

	//Libero su memoria
	free(proceso);
}

//============================================ FUNCIONES DE ASIGNACION DE MARCOS/PAGINAS =============================================

int cantidad_de_marcos_libres(){
	
	t_marco* marco_obtenido = NULL;
	int contador=0;
	
	for(int i = 0; i<list_size(marcos); i++){
		marco_obtenido = list_get(marcos,i);
		if(marco_obtenido->libre == 1){
			contador++;
		}
	}
	return contador;
}

int cantidad_de_marcos_necesarios(int tamanio_contenido_bytes){
	return (int) ceil(tamanio_contenido_bytes/tam_pagina); //PROBAR CASTEO
}

void quitar_marcos_a_proceso(t_proceso_en_memoria* proceso, uint32_t cantidad_marcos_a_liberar){

	log_info(memoria_logger, "Liberando paginas - PID: %d - Tamaño: %d", proceso->pid, cantidad_marcos_a_liberar); 
	
	//TODO preguntar si borramos las paginas cuando se hace resize menor o en que cambia si las dejamos con un bit no cargadas

	for(int i = 0; i<cantidad_marcos_a_liberar; i++){
		t_pagina *pagina_removida = malloc(sizeof(t_pagina));
		int tamanio_tabla_paginas = list_size(proceso->paginas_en_memoria) - 1;
		//Saca desde el final las paginas
		pagina_removida = list_remove(proceso->paginas_en_memoria, tamanio_tabla_paginas); 
		liberar_marco(pagina_removida->nro_marco);

		free(pagina_removida);
	}
}

void liberar_marco(int marco_a_liberar){
	
	t_marco *marco = buscar_marco_por_numero(marco_a_liberar);  

	marco->nro_pagina = -1;
	marco->libre = 1;
	marco->pid_proceso = -1;
}

t_marco* buscar_marco_por_numero(int numero_de_marco) {
    
    for (int i = 0; i < list_size(marcos); i++) {
        t_marco* marco_actual = (t_marco*) list_get(marcos, i);
        
		if (marco_actual->nro_marco == numero_de_marco) {
            
            return marco_actual;
        }
    }
    return NULL;
}

void asignar_marcos_a_proceso(t_proceso_en_memoria* proceso, int cantidad_de_marcos_necesarios) {
	
	//PRIMERO DEBERIA BUSCAR MARCOS LIBRES
	//SEGUNDO AL MARCO LIBRE ASIGNARLE EL PID DEL PROCESO Y MARCARLO COMO OCUPADO 
	//AL PROCESO AGREGARLE UNA PAGINA Y A ESA PAGINA ASIGNARLE EL MARCO Y EL PID
	//SEGUIR RECORRIENDO LA LISTA DE MARCOS LIBRES MIENTRAS CONTADOR SEA MENOR A CANT_MARCOS_NECESARIOS
	
	t_marco* marco_obtenido = NULL;
	int contador=0;

	log_info(memoria_logger, "Agregando paginas - PID: %d - Tamaño: %d marcos", proceso->pid, cantidad_de_marcos_necesarios); 

	for(int i = 0; i<list_size(marcos); i++){
		if(contador < cantidad_de_marcos_necesarios){
			marco_obtenido = (t_marco*) list_get(marcos,i);
			
			if(marco_obtenido->libre == 1){ //SI EL MARCO QUE SE OBTIENE ESTA LIBRE
				asignar_proceso_a_marco(proceso, marco_obtenido); //TAMBIEN AGREGA PAGINA A PROCESO
				contador++;
			}
		}
	}
}

void asignar_proceso_a_marco(t_proceso_en_memoria* proceso, t_marco* marco){
	
	agregar_pagina_a_proceso(proceso, marco);
	
	marco->pid_proceso = proceso->pid;
	marco->nro_pagina = list_size(proceso->paginas_en_memoria)-1;
	marco->libre = 0;
}

//Busca en los procesos cuyas instrucciones ya fueron cargadas
t_proceso_en_memoria* obtener_proceso_en_memoria(int pid) { 
    
	for (int i = 0; i < list_size(procesos_en_memoria); i++) {
        t_proceso_en_memoria* proceso = (t_proceso_en_memoria*) list_get(procesos_en_memoria, i);
        if (proceso->pid == pid) {
            return proceso; 
        }
    }
	return NULL;
}

void agregar_pagina_a_proceso(t_proceso_en_memoria* proceso, t_marco* marco){
	
	t_pagina *pagina_nueva = malloc(sizeof(t_pagina));

	pagina_nueva->pid_proceso = proceso->pid;
	pagina_nueva->nro_marco = marco->nro_marco;
	pagina_nueva->nro_pagina = list_size(proceso->paginas_en_memoria)-1;

	list_add(proceso->paginas_en_memoria, pagina_nueva);
}

uint32_t buscar_marco(uint32_t numero_pagina, int pid){
    
	//Obtengo el proceso por el pid
    t_proceso_en_memoria *proceso = obtener_proceso_en_memoria(pid);

    // Busco la página correspondiente al número de página
    t_pagina *pagina = NULL;

    for (int i = 0; i < list_size(proceso->paginas_en_memoria) && (pagina == NULL); i++) {
        t_pagina *pagina_actual = (t_pagina*) list_get(proceso->paginas_en_memoria, i);
       
	    if (pagina_actual->nro_pagina == numero_pagina) {
            pagina = pagina_actual;
            //break;
        }
    }

    if (pagina == NULL) {
        log_error(memoria_logger, "No se encontró la página %d en el proceso con PID: %d\n", numero_pagina, pid);
        //return -1; 
    }

    // Obtengo el número de marco asignado a la página
    int marco = pagina->nro_marco;
    if (marco == -1) {
        log_info(memoria_logger, "No hay marco asignado para la página %d en el proceso con PID: %d\n", numero_pagina, pid);
    } else {
        log_info(memoria_logger, "PID: %d - OBTENER MARCO - Página: %d - Marco: %d\n", pid, numero_pagina, marco);
    }

    return marco;
}

/* // MUESTREA PARA TESTEAR MEMORIA -> NO USAR EN EL TP 
static void mostrar_procesos_en_memoria() {
    printf("=== Procesos en memoria ===\n");
    for (int i = 0; i < list_size(procesos_en_memoria); i++) {
        t_proceso_en_memoria* proceso = (t_proceso_en_memoria*)list_get(procesos_en_memoria, i);
        mostrar_proceso_en_memoria(proceso);
    }
    printf("\n");
}

static void mostrar_marcos() {
    printf("=== Marcos ===\n");
    for (int i = 0; i < list_size(marcos); i++) {
        t_marco* marco = (t_marco*)list_get(marcos, i);
        mostrar_marco(marco);
    }
    printf("\n");
}

static void mostrar_contenido() {
    uint8_t *ptr = (uint8_t*)espacio_usuario;

    for (size_t i = 0; i < config_valores_memoria.tam_memoria; i++) {
        if (ptr[i] >= 32 && ptr[i] <= 126) {
            printf("%c", ptr[i]); // Carácter imprimible
        } else {
            printf("%u", ptr[i]); // Valor numérico
        }
    }
    printf("\n");
}
*/

//================================================== MANEJO ESCRITURAS EN PAGINAS/MARCOS ==================================================

static void escribir_contenido_en_partes(t_proceso_en_memoria* proceso, uint32_t direccion_fisica, uint32_t tamanio_escritura, void* contenido);
static bool contenido_cabe_en_marcos(t_proceso_en_memoria* proceso, int tamanio_contenido_bytes);
static int desplazamiento(uint32_t direccion_fisica);
static int lugar_restante_en_marco(uint32_t direccion_fisica);
static uint32_t calcular_direccion_fisica_inicial_marco(t_marco* marco);
static bool comparador_paginas(void* pagina1, void* pagina2);
static void ordenar_lista_por_numero_pagina(t_list* tabla_paginas);
static void copiar_contenido_en_partes_espacio_usuario(uint32_t direccion_fisica_inicial, int bytes_restantes_primer_marco, void* contenido, uint32_t tam_contenido, t_list* paginas_contiguas);
static t_list* obtener_paginas_contiguas(t_proceso_en_memoria* proceso, int cantidad_paginas, uint32_t pagina_inicio);



void escribir_contenido_espacio_usuario(int pid, uint32_t direccion_fisica, uint32_t tamanio_escritura, void* contenido){
	
	//TODO METODO ACCESO ESPACIO USUARIO CADA VEZ Q SE HACE MEMCPY

	t_proceso_en_memoria* proceso = obtener_proceso_en_memoria(pid);

	if(!contenido_cabe_en_marcos(proceso, tamanio_escritura)){
		//TODO preguntar que onda, igual dudo que se de este caso
	}

	//Entra en una pagina/marco
	if (lugar_restante_en_marco(direccion_fisica) >= tamanio_escritura){
		
		//Entra en este marco
			
		memcpy(espacio_usuario + direccion_fisica, contenido, tamanio_escritura); 
		

		//memcpy(&valor, espacio_usuario + direccion_fisica, tamanio_escritura); 
		
		//slog_info(memoria_logger, "VALOR LEIDOO: %d", valor);
	}else{

		escribir_contenido_en_partes(proceso, direccion_fisica, tamanio_escritura, contenido);

	}

}

static void escribir_contenido_en_partes(t_proceso_en_memoria* proceso, uint32_t direccion_fisica, uint32_t tamanio_escritura, void* contenido){
	
	//Spliteo el contenido segun en cuantas divisiones de paginas/marcos deba hacerse 

	t_marco* primer_marco = marco_desde_df(direccion_fisica);
	
	int bytes_restantes_primer_marco = lugar_restante_en_marco(direccion_fisica); //Cuantos bytes entran del contenido en el primer marco marcado por df

	int cantidad_paginas_necesarias = cantidad_de_marcos_necesarios(tamanio_escritura - bytes_restantes_primer_marco) +1; //Cuantas paginas/marcos se necesitan para el resto del contenido, el +1 es de la primera que ya restamos los bytes

	t_list* paginas_a_cargar = obtener_paginas_contiguas(proceso, cantidad_paginas_necesarias, primer_marco->nro_pagina);

	//Ahora ya tenemos las paginas en las cuales de divididara el contenido, la primera es la que hay que chequear cuantos bytes escribir, el resto enteras
	
	copiar_contenido_en_partes_espacio_usuario(direccion_fisica, bytes_restantes_primer_marco, contenido, tamanio_escritura, paginas_a_cargar);

	list_destroy_and_destroy_elements(paginas_a_cargar, free);
}

static bool contenido_cabe_en_marcos(t_proceso_en_memoria* proceso, int tamanio_contenido_bytes) {
    
	int cantidad_paginas_necesarias = cantidad_de_marcos_necesarios(tamanio_contenido_bytes);

    // Verificar si la cantidad de páginas necesarias cabe en las páginas cargadas en los marcos del proceso
    int cantidad_paginas_cargadas = list_size(proceso->paginas_en_memoria);

	//No importa lo que tengan ya que los procesos van a pisar sus escrituras manejadas con DLS
    return cantidad_paginas_cargadas >= cantidad_paginas_necesarias;

}

int numero_marco(uint32_t direccion_fisica){
	return floor(direccion_fisica / tam_pagina);
}

static int desplazamiento(uint32_t direccion_fisica){
	return direccion_fisica%tam_pagina;
}

static int lugar_restante_en_marco(uint32_t direccion_fisica){
	
	int offset = desplazamiento(direccion_fisica);
	
	return tam_pagina-offset;
}

// Calcula la dirección física usando el número de marco y el tamaño de página
static uint32_t calcular_direccion_fisica_inicial_marco(t_marco* marco) {
        
	uint32_t direccion_fisica_a_retornar = marco->nro_marco * tam_pagina;
    
	return direccion_fisica_a_retornar;
}


static bool comparador_paginas(void* pagina1, void* pagina2) {
        
		t_pagina* p1 = (t_pagina*) pagina1;
        t_pagina* p2 = (t_pagina*) pagina2;
        return p1->nro_pagina < p2->nro_pagina;
}

static void ordenar_lista_por_numero_pagina(t_list* tabla_paginas){
    
    // Ordena la lista de páginas en memoria por número de página ascendente
    list_sort(tabla_paginas, comparador_paginas);
}

static void copiar_contenido_en_partes_espacio_usuario(uint32_t direccion_fisica_inicial, int bytes_restantes_primer_marco, void* contenido, uint32_t tam_contenido, t_list* paginas_contiguas) {
    
	uint32_t bytes_copiados = 0;
	uint32_t bytes_por_marco = tam_pagina;

	//Trato la primer pagina por si no comienza en 0 la DF y hay que escribir desde la mitad del marco
	
	//Copio solo la parte que entra en el primer marco
	memcpy(espacio_usuario + direccion_fisica_inicial, &contenido, bytes_restantes_primer_marco);
	
	bytes_copiados += bytes_restantes_primer_marco; 


    // Iterar sobre las páginas contiguas
    for (int i = 1; i < list_size(paginas_contiguas) && (bytes_copiados <= tam_contenido); i++) { // (bytes_copiados <= tam_contenido) ->orque puede no llegar a copiar completa la ultima pagina
        
		t_pagina* pagina = list_get(paginas_contiguas, i);
        
		t_marco* marco = buscar_marco_por_numero(pagina->nro_marco);
        
		//HASTA ACA LLEGAMOS - 26/4

        // Calcular la dirección física dle marco actual en donde comenzar a escribir la parte
        uint32_t direccion_fisica = calcular_direccion_fisica_inicial_marco(marco);
		
		bytes_por_marco = tam_pagina - (direccion_fisica % tam_pagina);
		if (bytes_por_marco > tam_contenido - bytes_copiados) {
   			bytes_por_marco = tam_contenido - bytes_copiados;
		} //VER ESTOOOOOO

		// Copiar los bytes en el espacio de usuario, al sumar bytes_copiados esta partiendo el string/entero
        memcpy(espacio_usuario + direccion_fisica, contenido + bytes_copiados, bytes_por_marco);

        // Actualizar el número de bytes copiados
        bytes_copiados += bytes_por_marco;
    }

}

static t_list* obtener_paginas_contiguas(t_proceso_en_memoria* proceso, int cantidad_paginas, uint32_t pagina_inicio){

	//Retorna la lista con las paginas en las cuales se va a cargar el contenido, a partir de la pagina que se pasa por parametro
	t_list* lista_paginas = list_create();

	for (int i = 0; i < cantidad_paginas; i++) {
        
		//Ordeno la tabla de paginas para acceder directamente con el index
		ordenar_lista_por_numero_pagina(proceso->paginas_en_memoria);

		// Busca la página correspondiente en la tabla de páginas del proceso
        int nro_pagina = pagina_inicio + i; //Al arrancar en 0 también agrega la primera, que hay que tratar a parte
        
		t_pagina* pagina = list_get(proceso->paginas_en_memoria, nro_pagina);

		if (pagina == NULL){
			//TODO
		}

        list_add(lista_paginas, pagina);
       
    }

    return lista_paginas;
}

/*

ALGORITMO FUNCIONAMIENTO DE LA ESCRITURA

NECESITO: DF, TAMAÑO A ESCRIBIR (sino lo calculo para enteros)

------------

1) EN EL CASO DE MOVIN-MOVOUT TENEMOS SERVIDO EL DATO PORQUE ES SOLO UN ENTERO QUE VIENE DEL REGISTRO
	EN EL CASO DE COPYSTRING -> BUSCAR LO QUE SE DEBE ESCRIBIR EN EL VOID* 
	
	EN LOS OTROS CASO VER!

------------

2)) VER SI ENTRA LO QUE SE QUIERE ESCRIBIR EN EL MARCO ASOCIADO

SI EL TAMAÑOCONTENIDO ES MENOR AL TAMPAG
	
	SI EL LUGAR RESTANTE EN EL MARCO ES <= AL TAMAÑOCONTENIDO (supera los bordes del marco)
		ESCRIBIR CONTENIDO EN DF -> en este caso se asegura de no pisar otro marco, si por ejemplo el desp es 
									el ultimo byte y toca un entero de 4bytes el prox marco que no necesariamente 
									es del proceso.
	
	SINO
		ESCRIBIR CONTENIDO EN PARTES

SINO
	
	ESCRIBIR CONTENIDO EN PARTES

------------

3)) ESCRIBIR CONTENIDO EN PARTES

BUSCAR DADA LA DF EL LUGAR RESTANTE EN MARCO (SI EL DESPLAZAMIENTO ES PJ 3, RESTAN TAMMARCO-DESPLAZAMIENTO BYTES EN EL MARCO, NO EL TOTAL DE BYTES)

BUSCAR CUANTOS MARCOS SE REQUIEREN ADEMAS DEL LUGAR RESTANTE EN EL PRIMER MARCO

BUSCAR MARCOS CONTIGUOS DEL PROCESO PARA ESCRIBIR LA DATA

DIVIDIR EL STRING/ENTERO 
	SE PUEDE USAR: char* parte_string = malloc(bytes_disponibles_en_marco + 1); // +1 para el carácter nulo '\0'
				   strncpy(parte_string, string, bytes_disponibles_en_marco);
				   parte_string[bytes_disponibles_en_marco] = '\0';

COPIAR CADA PARTE DEL STRING EN LA DF DEL MARCO QUE SE ESTE COPIANDO
YA QUE EN EL PRIMER MARCO TENEMOS LA DF QUE VIENE DE CPU
PERO EN LOS SIGUIENTES HAY QUE VER DONDE ARRANCAN, ES DECIR LA DF DE LA PRIMERA POSICION
	DF_inicio_marco = NUMMARCO * TAMPAGINA;
	memcpy(espacio_usuario + direccion_fisica, parte_string, tamaño_parte_string);
        
*/

void* leer_contenido_espacio_usuario(int pid, uint32_t direccion_fisica, uint32_t tamanio_lectura){
    
	t_proceso_en_memoria* proceso = obtener_proceso_en_memoria(pid);

    // Verificar si el contenido a leer cabe en los marcos del proceso
    if (!contenido_cabe_en_marcos(proceso, tamanio_lectura)){
        // Manejar caso de error, por ejemplo, no se puede leer todo el contenido solicitado
        return NULL;
    }

    // Calculamos la cantidad de páginas necesarias para la lectura
    int cantidad_paginas_necesarias = cantidad_de_marcos_necesarios(tamanio_lectura);

    // Obtener el número de página inicial
    t_marco* marco_inicial = marco_desde_df(direccion_fisica);
	int pagina_inicial = marco_inicial->nro_pagina;

    // Obtener la lista de páginas contiguas a leer
    t_list* paginas_a_leer = obtener_paginas_contiguas(proceso, cantidad_paginas_necesarias, pagina_inicial);

    // Crear un buffer para almacenar el contenido leído
    void* contenido_leido = malloc(tamanio_lectura);

    // Verificar si la dirección física inicial tiene un desplazamiento
    int desplazamiento_inicial = desplazamiento(direccion_fisica);

    uint32_t bytes_a_leer = tam_pagina - desplazamiento_inicial;

    // Leer lo que quede del primer marco, puede ser todo o una parte si el desp era mas q 0
    memcpy(contenido_leido, espacio_usuario + direccion_fisica, bytes_a_leer);

    // Actualizar la cantidad de bytes leídos
    uint32_t bytes_leidos = bytes_a_leer;

    // Iterar sobre las páginas contiguas para continuar la lectura
    for (int i = 1; i < list_size(paginas_a_leer) && (bytes_leidos < tamanio_lectura); i++) {
        
		t_pagina* pagina = list_get(paginas_a_leer, i);
        t_marco* marco = buscar_marco_por_numero(pagina->nro_marco);

        // Calcular la dirección física inicial del marco actual
        uint32_t direccion_fisica_marco = calcular_direccion_fisica_inicial_marco(marco);

        // Calcular cuántos bytes se pueden leer en esta página/marco, en la ultima pueden ser menos q el tampag
        uint32_t bytes_por_leer = tam_pagina - (direccion_fisica_marco % tam_pagina);
		if (bytes_por_leer > tamanio_lectura - bytes_leidos) {
   			bytes_por_leer = tamanio_lectura - bytes_leidos;
		}

        // Leer desde la dirección física del marco actual
        memcpy(contenido_leido + bytes_leidos, espacio_usuario + direccion_fisica_marco, bytes_a_leer);

        // Actualizar la cantidad de bytes leídos
        bytes_leidos += bytes_a_leer;
    }

    // Liberar la lista de páginas a leer
    list_destroy_and_destroy_elements(paginas_a_leer, free);

    return contenido_leido;
}



