#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plan_console.h"
#include "commands.h"
#include <readline/readline.h>
#include <readline/history.h>

void * planner_console_launcher() {
	char *linea, *param1, *param2;
	int command_number, quit = 0;
	const char* command_list[] = {"salir", "pausa", "reanudar",
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
	case PAUSE:
		if(!param1 && !param2) pause();
		else {printf("El comando 'pausa' no recibe parametros!\n");}
		break;
	case RESUME:
		if(!param1 && !param2) resume();
		else {printf("El comando 'resumir' no recibe parametros!\n");}
		break;
	case BLOCK:
		if(param1 && param2) block(param1, param2);
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
