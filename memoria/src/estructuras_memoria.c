#include "memoria.h"

//============================================ PROCESOS ====================================================
t_proceso_en_memoria* crear_estructuras_memoria(int pid, FILE* archivo){
    char *instruccion = NULL;
	size_t longitud = 0;

	t_proceso_en_memoria *proceso = malloc(sizeof(t_proceso_en_memoria));
	proceso->pid = pid;
	proceso->instrucciones = list_create();

    //leo cada una de las lineas del archivo
	while (getline(&instruccion, &longitud, archivo) != -1)
	{
        //separo las lineas que lei, cada linea va a ser una instruccion
		if (strcmp(instruccion, "\n"))
		{
			int longitud = strlen(instruccion);
			if (instruccion[longitud - 1] == '\n')
				instruccion[longitud - 1] = '\0';

            //las instrucciones separadas las guardo como char en la lista de instrucciones del proceso
			char* aux = malloc(strlen(instruccion)+1);
			strcpy(aux,instruccion);
			list_add(proceso->instrucciones, aux);
		} 
	}
	free(instruccion);
	fclose(archivo);

    //muestro las instrucciones a ejecutar del proceso
	for(int i=0; i<list_size(proceso->instrucciones); i++){
		char* aux= list_get(proceso->instrucciones, i);
		printf("%s\n",aux);
	}

	list_add(procesos_en_memoria, proceso);
	return proceso;
}

char* buscar_instruccion_proceso(int program_counter, int pid){
	t_proceso_en_memoria *proceso = buscar_proceso(procesos_en_memoria, pid);
	char *instruccion = list_get(proceso->instrucciones, program_counter);
	return instruccion;
}

t_proceso_en_memoria *buscar_proceso(t_list *lista, int pid_buscado){
	int elementos = list_size(lista);
	for (int i = 0; i < elementos; i++)
	{
		t_proceso_en_memoria *proceso = list_get(lista, i);
		if (pid_buscado == proceso->pid)
		{
			return proceso;
		}
	}
	log_error(memoria_logger, "Proceso no encontrado\n");
	return NULL;
}