#include "console.h"

int RUNNING_FLAG = 1;

/*Commands*/
void pause(){
	RUNNING_FLAG = 0;
	printf("Planificación pausada\n");
};
void resume(){
	RUNNING_FLAG = 1;
	printf("Planificación reanudada\n");
};
void block(char* key, int id){ //Issue #1105 TODO No se entiende la diferencia entre ambos casos
	if(is_key_free(key)) {
		ESIRegistration * esi = search_esi(id);
		ResourceAllocation * ra = malloc(sizeof(ResourceAllocation));
		ra->esi_id = -1;
		strcpy(ra->key, key);
		ra->type = BLOCKED;
		if(esi != NULL) {
			ra->esi_id = id;
			while(esi->status == S_RUNNING) { } //Espero a que deje de correr
			esi->status = S_BLOCKED;
			add_esi_to_blocked(esi);
			printf("El ESI ahora espera la clave\n");
		} else {
			printf("El ESI no existe. Se bloqueó la key como init.\n");
		}
		process_allocation(ra);
	} else {
		ESIRegistration * esi = search_esi(id);
		ResourceAllocation * ra = malloc(sizeof(ResourceAllocation));
		ra->esi_id = -1;
		strcpy(ra->key, key);
		ra->type = WAITING;
		if(esi != NULL) {
			ra->esi_id = id;
			while(esi->status == S_RUNNING) { } //Espero a que deje de correr
			esi->status = S_BLOCKED;
			add_esi_to_blocked(esi);
			printf("El ESI ahora espera la clave\n");
			process_allocation(ra);
		} else {
			printf("El ESI no existe.\n");
		}
	}
};
void unblock(char* key){
	unlock_resource_all(key);
	printf("Recurso desbloqueado.\n");
};
void list(char* resource){
	int a;
	t_list * list = search_esis_waiting_for(resource);
	if(list->elements_count == 0) {
		printf("No hay ningún ESI esperando por el recurso %s.\n", resource);
	} else {
		for(a=0 ; a<list->elements_count ; a++) {
			ResourceAllocation * ra = list_get(list, a);
			printf("ESI %d.\n", ra->esi_id);
		}
		free(list);
	}
};
void kill(char* id){
};
void status(char* key){
};
void deadlock(){
};
void exit_c(){
};

void info(){
    printf("pausar/continuar: El Planificador no le dará nuevas órdenes de ejecución a ningún ESI mientras se encuentre pausado.\n");
    printf("bloquear <clave> <ID>: Se bloqueará el proceso ESI hasta ser desbloqueado (ver más adelante), especificado por dicho ID(^3) en la cola del recurso clave. Vale recordar que cada línea del script a ejecutar es atómica, y no podrá ser interrumpida; si no que se bloqueará en la próxima oportunidad posible. Solo se podrán bloquear de esta manera ESIs que estén en el estado de listo o ejecutando.\n");
    printf("desbloquear <clave>: Se desbloqueara el primer proceso ESI bloquedo por la clave especificada. Solo se bloqueará ESIs que fueron bloqueados con la consola. Si un ESI está bloqueado esperando un recurso, no podrá ser desbloqueado de esta forma.\n");
    printf("listar <recurso>: Lista los procesos encolados esperando al recurso.\n");
    printf("kill <ID>: finaliza el proceso. Recordando la atomicidad mencionada en “bloquear”.\n");
    printf("status <clave>: Debido a que para la correcta coordinación de las sentencias de acuerdo a los algoritmos de distribución(^4) se requiere de cierta información sobre las instancias del sistema.\n");
    printf("deadlock: Esta consola también permitirá analizar los deadlocks que existan en el sistema y a que ESI están asociados. Pudiendo resolverlos manualmente con la sentencia de kill previamente descrita.\n");
}


/*Console*/

int get_running_flag() {
	return RUNNING_FLAG;
}

void * planner_console_launcher() {
	char *linea, *param1, *param2;
	int command_number, quit = 0;
	const char* command_list[] = {"salir", "pausar", "reanudar",
			"bloquear", "desbloquear", "listar", "kill",
			"status", "deadlock", "info"};
	int command_list_length = (int) sizeof(command_list) /
			sizeof(command_list[0]);

	printf("Bienvenido/a a la consola del planificador\n");
	printf("Escribi 'info' para obtener una lista de comandos\n");

	while(quit == 0){
		linea = readline("> ");
		add_history(linea);
		string_tolower(linea);
		if(parse(&linea, &param1, &param2)) printf("Demasiados parámetros!\n");
		else {
			command_number = find_in_array(linea,
					command_list, command_list_length);

			command_number == EXIT ? quit = 1 : execute(command_number,
					param1, param2);
		}

		free(linea);
	}
	return EXIT_SUCCESS;
}

int parse(char **linea, char **param1, char **param2){
	//separo el input en palabras
	strtok(*linea, " ");
	*param1 = strtok(NULL, " ");
	*param2 = strtok(NULL, " ");
	if(strtok(NULL, " ")) return -1; //en este caso hay mas de 2 parametros
	return 0;
}

void execute(int command_number, char* param1, char* param2){
	switch(command_number){
	case -1:
		printf("Comando no reconocido\n");
		break;
	case EXIT:
		if(!param1 && !param2) exit_c();
		else {printf("El comando 'salir' no recibe parametros!\n");}
		break;
	case PAUSE:
		if(!param1 && !param2) pause();
		else {printf("El comando 'pausa' no recibe parametros!\n");}
		break;
	case RESUME:
		if(!param1 && !param2) resume();
		else {printf("El comando 'resumir' no recibe parametros!\n");}
		break;
	case BLOCK:
		if(param1 && param2) block(param1, atoi(param2));
		else {printf("El comando 'bloquear' recibe dos parametros!\n");}
		break;
	case UNBLOCK:
		if(param1 && !param2) unblock(param1);
		else {printf("El comando 'desbloquear' recibe un parametro!\n");}
		break;
	case LIST:
		if(param1 && !param2) list(param1);
		else {printf("El comando 'listar' recibe un parametro!\n");}
		break;
	case KILL:
		if(param1 && !param2) kill(param1);
		else {printf("El comando 'kill' recibe un parametro!\n");}
		break;
	case STATUS:
		if(param1 && !param2) status(param1);
		else {printf("El comando 'status' recibe un parametro!\n");}
		break;
	case DEADLOCK:
		if(!param1 && !param2) resume();
		else {printf("El comando 'deadlock' no recibe parametros!\n");}
		break;
	case INFO:
		if(!param1 && !param2) info();
		else {printf("El comando 'info' no recibe parametros!\n");}
		break;
	}
}


int find_in_array(char* linea, const char** command_list, int length){
	for(int i = 0; i < length; i++)
		if(strcmp(linea, command_list[i]) == 0) return i;
	return -1;
}

void string_tolower(char* string){
	for(int i = 0; string[i]; i++)
		string[i] = tolower(string[i]);
}