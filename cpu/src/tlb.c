#include "cpu.h"

int tiempo = 0;
int tiempo_carga = 0;
pthread_mutex_t mutex_tiempo;
pthread_mutex_t mutex_tiempo_carga;

static int obtener_tiempo();
static int obtener_tiempo_carga();
static int buscar_indice_entrada_menor_uso(t_list* lista);
static int buscar_indice_entrada_mas_vieja(t_list* lista);
static bool menor_uso(t_entrada* siguiente_entrada, t_entrada* menor_entrada);
static bool mas_vieja(t_entrada* siguiente_entrada, t_entrada* menor_entrada);
static void reemplazo_por_LRU(t_entrada* nueva_entrada);
static void reemplazo_por_FIFO(t_entrada* nueva_entrada);
//====================================================================================================================

uint32_t consultar_tlb(int pid, uint32_t pagina){

    if(cantidad_entradas_tlb == 0){
        //tlb deshabilitada
        return -2;
    }else {
        int tamanio_actual_tlb = list_size(tlb);

        //HIT: MARCO
        for (int i = 0; i < tamanio_actual_tlb; i++) {
            
            t_entrada* entrada_actual = (t_entrada*) list_get(tlb, i);

            if (entrada_actual->pid == pid && entrada_actual->pagina == pagina) {

                //Uso la entrada, por ende en FIFO no pasa nada, en LRU debo mover la entrada al tope de la cola
                if(strcmp(algoritmo_tlb, "LRU") == 0){
                    entrada_actual->ultimo_uso = obtener_tiempo();
                }
                return entrada_actual->marco;
            }
        }

        //MISS: -1
        return -1;
    }
} 

static int obtener_tiempo(){
	pthread_mutex_lock(&mutex_tiempo);
	tiempo++;
	pthread_mutex_unlock(&mutex_tiempo);
	return tiempo;
}

static int obtener_tiempo_carga(){
	pthread_mutex_lock(&mutex_tiempo);
	tiempo_carga++;
	pthread_mutex_unlock(&mutex_tiempo);
    return tiempo_carga;
}
	
void agregar_entrada_tlb(int pid, uint32_t pagina, uint32_t marco){

    int tamanio_actual_tlb = list_size(tlb);

    t_entrada* nueva_entrada = (t_entrada*) malloc(sizeof(t_entrada));
    nueva_entrada->pid = pid;
    nueva_entrada->pagina = pagina;
    nueva_entrada->marco = marco;
    nueva_entrada->ultimo_uso = obtener_tiempo(); //se va modificando cada vez que la referencio
    nueva_entrada->tiempo_carga = obtener_tiempo_carga(); //me da el tiempo y no se modifica una vez que se asigna

    if( (tamanio_actual_tlb < cantidad_entradas_tlb) ){
        //No importa el algoritmo siempre encolo
        list_add(tlb, nueva_entrada);
        imprimir_tlb(tlb);

    }else{ 
        //porque puede estar deshabilitada la tlb
        if( (cantidad_entradas_tlb != 0) ){

            //REEMPLAZO DE ENTRADA 
            if(strcmp(algoritmo_tlb,"LRU") == 0){
                reemplazo_por_LRU(nueva_entrada);
            } else {
                reemplazo_por_FIFO(nueva_entrada);
            } 
            imprimir_tlb(tlb);
        }
    }
}

//las entradas con numero mas alta se referenciaron hace poco (u=1 se uso hace mas tiempo que u=8)
static bool menor_uso(t_entrada* siguiente_entrada, t_entrada* menor_entrada) {
    return siguiente_entrada->ultimo_uso < menor_entrada->ultimo_uso;
}

//las paginas con numero mas chico van primero (t=1 se cargo antes que t=8)
static bool mas_vieja(t_entrada* siguiente_entrada, t_entrada* mas_vieja){
    return (siguiente_entrada->tiempo_carga < mas_vieja->tiempo_carga); 
}

static int buscar_indice_entrada_menor_uso(t_list* lista) {

    //agarro la primera y voy comparando con el resto
    t_entrada* menor = list_get(lista, 0);
    int indice_menor_uso = 0;

    for (int i = 1; i < list_size(lista); i++) {
        t_entrada* siguiente_entrada = list_get(lista, i);
        if (menor_uso(siguiente_entrada, menor)) {
            menor = siguiente_entrada;
            indice_menor_uso = i;
        }
    }
    return indice_menor_uso;
}

static int buscar_indice_entrada_mas_vieja(t_list* lista) {

    //agarro la primera y voy comparando con el resto
    t_entrada* entrada_mas_vieja = list_get(lista, 0);
    int indice_mas_vieja = 0;

    for (int i = 1; i < list_size(lista); i++) {
        t_entrada* siguiente_entrada = list_get(lista, i);
        if (mas_vieja(siguiente_entrada, entrada_mas_vieja)) {
            entrada_mas_vieja = siguiente_entrada;
            indice_mas_vieja = i;
        }
    }
    printf("reemplazo fifo indice %d\n", indice_mas_vieja);
    return indice_mas_vieja;
}

static void reemplazo_por_LRU(t_entrada* nueva_entrada){

    //saco la entrada menos usada
    int indice_menos_usada = buscar_indice_entrada_menor_uso(tlb);
    list_remove(tlb, indice_menos_usada);

    //pongo en el indice que saque, la nueva entrada
    list_add_in_index(tlb, indice_menos_usada, nueva_entrada);
    printf("reemplazo lru indice %d\n", indice_menos_usada);
}

static void reemplazo_por_FIFO(t_entrada* nueva_entrada){
    
    int indice_mas_vieja = buscar_indice_entrada_mas_vieja(tlb);
    list_remove(tlb, indice_mas_vieja);

    //pongo en el indice que saque, la nueva entrada
    list_add_in_index(tlb, indice_mas_vieja, nueva_entrada);
}

void imprimir_tlb(t_list* tlb) { //SOLO PARA TESTING
    
    if (tlb == NULL) {
        printf("TLB vacía\n");
        return;
    }

    t_list_iterator *iterator = list_iterator_create(tlb);
    
    int index = 0;

    while (list_iterator_has_next(iterator)) {
        
        t_entrada *entrada = list_iterator_next(iterator);
        printf("Posicion TLB: %d - PID: %d - Pagina: %d - Marco: %d\n", index, entrada->pid, entrada->pagina, entrada->marco);
        index++;
    }

    list_iterator_destroy(iterator);
}