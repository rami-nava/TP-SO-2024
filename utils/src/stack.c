//PILA PARA USAR EN LRU BASADA EN LAS ACCIONES DE LAS LISTAS DE LAS COMMONS

#include "stack.h"

stack* stack_create() {
    stack* new_stack = malloc(sizeof(stack));
    if (new_stack != NULL) {
        new_stack->list = list_create();
    }
    return new_stack;
}

void stack_destroy(stack* s) {
    if (s != NULL) {
        list_destroy_and_destroy_elements(s->list, free);
        free(s);
    }
}

void stack_push(stack* s, void* element) {
    list_add(s->list, element);
}

void* stack_pop(stack* s) {
    if (stack_is_empty(s)) {
        return NULL;
    }
    return list_remove(s->list, list_size(s->list) - 1);
}

bool stack_is_empty(stack* s) {
    return list_is_empty(s->list);
}

int stack_size(stack* s) {
    return list_size(s->list);
}

void* stack_top(stack* s) {
    if (stack_is_empty(s)) {
        return NULL;
    }
    return list_get(s->list, list_size(s->list) - 1);
}

void* stack_get_at(stack* s, int index) {
    if (index < 0 || index >= stack_size(s)) {
        return NULL; // Índice fuera de rango
    }
    t_list_iterator* iterator = list_iterator_create(s->list);
    while (list_iterator_has_next(iterator) && index > 0) {
        list_iterator_next(iterator);
        index--;
    }
    void* element = list_iterator_next(iterator);
    list_iterator_destroy(iterator);
    return element;
}

void* stack_remove_at(stack* s, int index) {
    if (index < 0 || index >= stack_size(s)) {
        return NULL; // Índice fuera de rango
    }
    t_list_iterator* iterator = list_iterator_create(s->list);
    while (list_iterator_has_next(iterator) && index > 0) {
        list_iterator_next(iterator);
        index--;
    }
    void* element = list_remove(s->list, list_iterator_index(iterator));
    list_iterator_destroy(iterator);
    return element;
}


/*
list_create: Crea una nueva lista vacía.
list_destroy: Destruye una lista, liberando la memoria ocupada por la lista pero no por los elementos contenidos en ella.
list_destroy_and_destroy_elements: Destruye una lista y sus elementos.
list_add: Agrega un elemento al final de la lista.
list_add_in_index: Agrega un elemento en una posición específica de la lista.
list_add_sorted: Agrega un elemento a una lista ordenada manteniendo el orden definido por un comparador.
list_get: Obtiene el contenido de una posición específica de la lista.
list_get_minimum: Obtiene el mínimo valor de la lista según un comparador.
list_get_maximum: Obtiene el máximo valor de la lista según un comparador.
list_take: Retorna una nueva lista con los primeros n elementos.
list_slice: Retorna una nueva lista con los elementos desde el índice indicado hasta el conteo especificado.
list_take_and_remove: Retorna una nueva lista con los primeros n elementos, eliminándolos de la lista original.
list_slice_and_remove: Retorna una nueva lista con los elementos desde el índice indicado hasta el conteo especificado, eliminándolos de la lista original.
list_filter: Retorna una nueva lista con los elementos que cumplen una condición.
list_map: Retorna una nueva lista con los elementos transformados.
list_flatten: Retorna una nueva lista con los elementos de una lista de listas.
list_replace: Reemplaza un elemento en una posición específica de la lista.
list_replace_by_condition: Reemplaza un elemento en la posición de la lista donde se cumpla una condición.
list_replace_and_destroy_element: Reemplaza un elemento en una posición de la lista y libera el valor anterior.
list_remove: Remueve un elemento de una posición específica de la lista y lo retorna.
list_remove_element: Remueve un elemento de la lista.
list_remove_and_destroy_element: Remueve y destruye un elemento de la lista.
list_remove_by_condition: Remueve el primer elemento que cumple una condición.
list_remove_and_destroy_by_condition: Remueve y destruye el primer elemento que cumple una condición.
list_remove_and_destroy_all_by_condition: Remueve y destruye todos los elementos que cumplen una condición.
list_clean: Elimina todos los elementos de la lista.
list_clean_and_destroy_elements: Elimina y destruye todos los elementos de la lista.
list_iterate: Itera sobre la lista aplicando una función a cada elemento.
list_find: Encuentra el primer elemento que cumple una condición.
list_size: Obtiene el tamaño de la lista.
list_is_empty: Verifica si la lista está vacía.
list_sort: Ordena la lista según un comparador.
list_sorted: Retorna una nueva lista ordenada según un comparador.
list_count_satisfying: Cuenta la cantidad de elementos que cumplen una condición.
list_any_satisfy: Determina si algún elemento cumple una condición.
list_all_satisfy: Determina si todos los elementos cumplen una condición.
list_duplicate: Crea una copia de la lista.
list_fold1: Aplica una operación a todos los elementos de la lista tomando al primero como semilla.
list_fold: Aplica una operación a todos los elementos de la lista.
list_iterator_create: Inicializa una iteración externa sobre la lista.
list_iterator_has_next: Verifica si quedan elementos por recorrer en la iteración.
list_iterator_next: Avanza al siguiente elemento en la iteración.
*/