#include "memoria.h"
//================================================== MANEJO ESCRITURAS EN PAGINAS/MARCOS ==================================================

static void escribir_contenido_en_partes(t_proceso_en_memoria* proceso, uint32_t direccion_fisica, uint32_t tamanio_escritura, void* contenido);
static bool contenido_cabe_en_marcos(t_proceso_en_memoria* proceso, int tamanio_contenido_bytes);
static int desplazamiento(uint32_t direccion_fisica);
static int lugar_restante_en_marco(uint32_t direccion_fisica);
static uint32_t calcular_direccion_fisica_inicial_marco(t_marco* marco);
static bool comparador_paginas(void* pagina1, void* pagina2);
static void copiar_contenido_en_partes_espacio_usuario(uint32_t direccion_fisica_inicial, int bytes_restantes_primer_marco, void* contenido, uint32_t tam_contenido, t_list* paginas_contiguas, int pid);
static t_list* obtener_paginas_contiguas(t_proceso_en_memoria* proceso, int cantidad_paginas, uint32_t pagina_inicio);
static void imprimir_contenido_memoria(void* puntero, size_t tamano);

// ****** FUNCIONES AUXILIARES ****** //

// Calcula el desplazamiento de una dirección fisica
static int desplazamiento(uint32_t direccion_fisica){
	int offset = direccion_fisica%tam_pagina;
	return offset;
}

// Calcula el lugar restante en el marco
static int lugar_restante_en_marco(uint32_t direccion_fisica){
	
	int offset = desplazamiento(direccion_fisica);

	int lugar_restante = tam_pagina - offset;
	
	return lugar_restante;
}

// Calcula la dirección física usando el número de marco y el tamaño de página
static uint32_t calcular_direccion_fisica_inicial_marco(t_marco* marco) {
        
	uint32_t direccion_fisica_a_retornar = marco->nro_marco * tam_pagina;
    
	return direccion_fisica_a_retornar;
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

void escribir_contenido_espacio_usuario(int pid, uint32_t direccion_fisica, uint32_t tamanio_escritura, void* contenido){
	
	t_proceso_en_memoria* proceso = obtener_proceso_en_memoria(pid);

	if(!contenido_cabe_en_marcos(proceso, tamanio_escritura)){
		log_error(memoria_logger, "No se puede leer el contenido solicitado");
        return;
	}

	//lugar restante del marco donde estoy
	int lugar_restante = lugar_restante_en_marco(direccion_fisica);

	//Entra en una pagina/marco
	if (lugar_restante_en_marco(direccion_fisica) >= tamanio_escritura){
		
		acceso_a_espacio_usuario(pid, "ESCRIBIR EN EL MISMO MARCO", direccion_fisica, tamanio_escritura);

		//Copio el contenido en el marco correspondiente			
		memcpy(espacio_usuario + direccion_fisica, contenido, tamanio_escritura); 

		//lo escribo y lo restorno para usarlo en el log
		t_marco* marco_escrito = ocupar_marco_con_contenido(direccion_fisica, tamanio_escritura);

		printf("\n espacio ocupado del marco:%d , espacio disponible: %d\n", marco_escrito->bytes_ocupados, lugar_restante);
		imprimir_contenido_memoria(espacio_usuario + direccion_fisica, tamanio_escritura);

		//    PARA TESTS
		//mem_hexdump(espacio_usuario, config_valores_memoria.tam_memoria);
				
	}else{
		escribir_contenido_en_partes(proceso, direccion_fisica, tamanio_escritura, contenido);
	}

	free(contenido);
}

//Spliteo el contenido segun en cuantas divisiones de paginas/marcos deba hacerse 
static void escribir_contenido_en_partes(t_proceso_en_memoria* proceso, uint32_t direccion_fisica, uint32_t tamanio_escritura, void* contenido){
	
	//marco donde no entra todo lo que quiero escribir
	t_marco* primer_marco = marco_desde_df(direccion_fisica);
	
	int bytes_restantes_primer_marco = lugar_restante_en_marco(direccion_fisica); //Cuantos bytes entran del contenido en el primer marco marcado por df

	int cantidad_paginas_necesarias = cantidad_de_marcos_necesarios(tamanio_escritura - bytes_restantes_primer_marco) +1; //Cuantas paginas/marcos se necesitan para el resto del contenido, el +1 es de la primera que ya restamos los bytes

	t_list* paginas_a_cargar = obtener_paginas_contiguas(proceso, cantidad_paginas_necesarias, primer_marco->nro_pagina);

	//Ahora ya tenemos las paginas en las cuales de divididara el contenido, la primera es la que hay que chequear cuantos bytes escribir, el resto enteras
	
	copiar_contenido_en_partes_espacio_usuario(direccion_fisica, bytes_restantes_primer_marco, contenido, tamanio_escritura, paginas_a_cargar, proceso->pid);

	list_destroy_and_destroy_elements(paginas_a_cargar, free);
}

static void copiar_contenido_en_partes_espacio_usuario(uint32_t direccion_fisica_inicial, int bytes_restantes_primer_marco, void* contenido, uint32_t tam_contenido, t_list* paginas_contiguas, int pid) {
    
	uint32_t bytes_copiados = 0;
	uint32_t bytes_por_marco = tam_pagina; 	//Trato la primer pagina por si no comienza en 0 la DF y hay que escribir desde la mitad del marco
	
	acceso_a_espacio_usuario(pid, "ESCRIBIR EN UNA PARTECITA DEL MARCO ANTERIOR", direccion_fisica_inicial, bytes_restantes_primer_marco);

	//Copio solo la parte que entra en el primer marco
	memcpy(espacio_usuario + direccion_fisica_inicial, &contenido, bytes_restantes_primer_marco);
	t_marco* marco_escrito = ocupar_marco_con_contenido(direccion_fisica_inicial, bytes_restantes_primer_marco);

	bytes_copiados += bytes_restantes_primer_marco; 

    // Iterar sobre las páginas contiguas
    for (int i = 1; i < list_size(paginas_contiguas) && (bytes_copiados <= tam_contenido); i++) { // (bytes_copiados <= tam_contenido) ->orque puede no llegar a copiar completa la ultima pagina
        
		t_pagina* pagina = list_get(paginas_contiguas, i);
        
		t_marco* marco = buscar_marco_por_numero(pagina->nro_marco);
        
		//HASTA ACA LLEGAMOS - 26/4

        // Calcular la dirección física dle marco actual en donde comenzar a escribir la parte
        uint32_t direccion_fisica = calcular_direccion_fisica_inicial_marco(marco);
		
		bytes_por_marco = tam_pagina - (desplazamiento(direccion_fisica));

		//si en el marco que agarre puedo escribir todo, dejo de buscar?
		if (bytes_por_marco > tam_contenido - bytes_copiados) {
			
			//los bytes restantes del marco
   			bytes_por_marco = tam_contenido - bytes_copiados;
		}//si el marco no tiene suficiente espacio sigo buscando

		acceso_a_espacio_usuario(pid, "ESCRIBIR LO RESTANTE EN OTRO MARCO", direccion_fisica, bytes_por_marco);

		// Copiar los bytes en el espacio de usuario, al sumar bytes_copiados esta partiendo el string/entero
        memcpy(espacio_usuario + direccion_fisica, contenido + bytes_copiados, bytes_por_marco);
		ocupar_marco_con_contenido(direccion_fisica, bytes_por_marco);

        // Actualizar el número de bytes copiados
        bytes_copiados += bytes_por_marco;
    }
}

static t_list* obtener_paginas_contiguas(t_proceso_en_memoria* proceso, int cantidad_paginas, uint32_t pagina_inicio){

	//Retorna la lista con las paginas en las cuales se va a cargar el contenido, a partir de la pagina que se pasa por parametro
	t_list* lista_paginas = list_create();

	for (int i = 0; i < cantidad_paginas; i++) {
        
		 // Ordeno la tabla de paginas por número de página ascendente
    	list_sort(proceso->paginas_en_memoria, comparador_paginas);

		// Busca la página correspondiente en la tabla de páginas del proceso
        int nro_pagina = pagina_inicio + i; //Al arrancar en 0 también agrega la primera, que hay que tratar a parte
        
		t_pagina* pagina = list_get(proceso->paginas_en_memoria, nro_pagina);

		if (pagina == NULL){
			log_error(memoria_logger, "Pagina nula");
        	return NULL;
		}

        list_add(lista_paginas, pagina);
       
    }

    return lista_paginas;
}

void leer_contenido_espacio_usuario(int pid, uint32_t direccion_fisica, uint32_t tamanio_lectura, int cliente, op_code operacion) {
    
	uint32_t bytes_restantes_a_leer = 0;
	uint32_t bytes_a_leer_siguiente_marco = 0;

	t_proceso_en_memoria* proceso = obtener_proceso_en_memoria(pid);

    // Verificar si el contenido a leer cabe en los marcos del proceso
    if (!contenido_cabe_en_marcos(proceso, tamanio_lectura)){
		log_error(memoria_logger, "No se puede leer el contenido solicitado");
        return;
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

    // Verificar si la dirección física inicial tiene un desplazamiento (si ya esta medio escrita)
    int desplazamiento_inicial = desplazamiento(direccion_fisica);

	uint32_t bytes_a_leer_primer_marco = marco_inicial->bytes_ocupados;

	// Voy a empezar a leer desde donde esta escrito
	char* puntero_de_direccion_fisica = espacio_usuario + desplazamiento_inicial;

	acceso_a_espacio_usuario(pid, "LEER", direccion_fisica, bytes_a_leer_primer_marco);

    // Leemos lo que quede del primer marco, puede una parte si el offset es mas que 0
    memcpy(contenido_leido, puntero_de_direccion_fisica, bytes_a_leer_primer_marco);

    // Obtenemos bytes restantes a leer (si quedan)
	if(bytes_a_leer_primer_marco < tamanio_lectura){
		bytes_restantes_a_leer = tamanio_lectura - bytes_a_leer_primer_marco;
	}

	// Iterar sobre las páginas contiguas para continuar la lectura apartir de la segunda pagina
    for (int i = 1; i < list_size(paginas_a_leer) && (bytes_restantes_a_leer < tamanio_lectura); i++) {
        
		t_pagina* pagina = list_get(paginas_a_leer, i);
        t_marco* marco = buscar_marco_por_numero(pagina->nro_marco);

        // Calcular la dirección física inicial del marco actual
        uint32_t direccion_fisica_marco = calcular_direccion_fisica_inicial_marco(marco);

		bytes_a_leer_siguiente_marco = marco->bytes_ocupados;

		char* puntero_de_siguiente_direccion_fisica = espacio_usuario + desplazamiento(direccion_fisica_marco);

		// Accedemos al espacio de usuario
		acceso_a_espacio_usuario(pid, "LEER", direccion_fisica_marco, bytes_a_leer_siguiente_marco);

        // Leer desde la dirección física del marco actual
        memcpy(contenido_leido + bytes_a_leer_siguiente_marco, puntero_de_siguiente_direccion_fisica, bytes_a_leer_siguiente_marco);
		
		// Obtenemos bytes restantes a leer (si quedan)
		if(bytes_a_leer_siguiente_marco < tamanio_lectura){
			bytes_restantes_a_leer = tamanio_lectura - bytes_a_leer_siguiente_marco;
		}
    }

    // Liberar la lista de páginas a leer
    list_destroy_and_destroy_elements(paginas_a_leer, free);

	imprimir_contenido_memoria(contenido_leido, tamanio_lectura);

	// Devolver el contenido leído
	if(operacion == PEDIDO_MOV_IN){
	t_paquete* paquete_mov_in = crear_paquete(RESULTADO_MOV_IN);
	agregar_bytes_a_paquete(paquete_mov_in, contenido_leido, tamanio_lectura);
	enviar_paquete(paquete_mov_in, cliente);
	} else if (operacion == REALIZAR_LECTURA){
	t_paquete* paquete = crear_paquete(RESULTADO_LECTURA);
	agregar_cadena_a_paquete(paquete, contenido_leido);
	enviar_paquete(paquete, cliente);
	} else {
	t_paquete* paquete_fs_write = crear_paquete(VALOR_LECTURA);
	agregar_bytes_a_paquete(paquete_fs_write, contenido_leido, tamanio_lectura);
	enviar_paquete(paquete_fs_write, cliente);
	}

	free(contenido_leido);
}


static void imprimir_contenido_memoria(void* puntero, size_t tamano) {
    uint8_t* bytes = (uint8_t*)puntero; // Convertir el puntero a uint8_t* para tratarlo como una secuencia de bytes
    printf("El contenido en memoria es: ");
    for (size_t i = 0; i < tamano; i++) {
        printf("%02x ", bytes[i]); // Imprimir cada byte en formato hexadecimal
    }
    printf("\n");
}

static bool contenido_cabe_en_marcos(t_proceso_en_memoria* proceso, int tamanio_contenido_bytes) {
    
	int cantidad_paginas_necesarias = cantidad_de_marcos_necesarios(tamanio_contenido_bytes);

    // Verificar si la cantidad de páginas necesarias cabe en las páginas cargadas en los marcos del proceso
    int cantidad_paginas_cargadas = list_size(proceso->paginas_en_memoria);

	//No importa lo que tengan ya que los procesos van a pisar sus escrituras manejadas con DLS
    return cantidad_paginas_cargadas >= cantidad_paginas_necesarias;
}

static bool comparador_paginas(void* pagina1, void* pagina2) {
        
		t_pagina* p1 = (t_pagina*) pagina1;
        t_pagina* p2 = (t_pagina*) pagina2;
        return p1->nro_pagina < p2->nro_pagina;
}

