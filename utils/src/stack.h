#ifndef STACK_H_
#define STACK_H_


#include <commons/collections/list.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
    t_list* list;
} stack;

stack* stack_create();
void stack_destroy(stack* s);
void stack_push(stack* s, void* element);
void* stack_pop(stack* s);
bool stack_is_empty(stack* s);
int stack_size(stack* s);
void* stack_top(stack* s);
void* stack_get_at(stack* s, int index);
void* stack_remove_at(stack* s, int index);

#endif 
