#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plan_console.h"
#include "commands.h"
#include <readline/readline.h>
#include <readline/history.h>

int main(void) {
	char *linea, *param1, *param2;
	int command_number, quit = 0;
	const char* command_list[] = {"salir", "pausa", "reanudar",
			"bloquear", "bloquear", "listar", "kill",
			"status", "deadlock", "info"};
	int command_list_length = (int) sizeof(command_list) /
			sizeof(command_list[0]);

	printf("Bienvenido/a a la consola del planificador\n");
	printf("Escribi 'info' para obtener una lista de comandos\n");

	while(quit == 0){
		linea = readline("> ");
		add_history(linea);
		parse(&linea, &param1, &param2);
		command_number = find_in_array(linea,
				command_list, command_list_length);

		command_number == EXIT ? quit = 1 : execute(command_number,
				param1, param2);

		free (linea);
	}
	return EXIT_SUCCESS;
}

void parse(char **linea, char **param1, char **param2){
	//separo el input en palabras
	strtok(*linea, " ");
	*param1 = strtok(NULL, " ");
	*param2 = strtok(NULL, " ");

}

void execute(int command_number, char* param1, char* param2){
	switch(command_number){
	case -1:
		printf("Comando no reconocido\n");
		break;
	case PAUSE:
		pause();
		break;
	case RESUME:
		resume();
		break;
	case BLOCK:
		if(param1 && param2) block(param1, param2);
		else {printf("El comando 'block' recibe dos parametros\n");}
		break;
	case UNBLOCK:
		if(param1) unblock(param1);
		else {printf("El comando 'unblock' recibe un parametro\n");}
		break;
	case LIST:
		if(param1) list(param1);
		else {printf("El comando 'list' recibe un parametro\n");}
		break;
	case KILL:
		if(param1) kill(param1);
		else {printf("El comando 'kill' recibe un parametro\n");}
		break;
	case STATUS:
		if(param1) status(param1);
		else {printf("El comando 'status' recibe un parametro\n");}
		break;
	case DEADLOCK:
		deadlock();
		break;
	case INFO:
		info();
		break;
	}
}


int find_in_array(char* linea, const char** command_list, int length){
	for(int i = 0; i < length; i++)
		if(strcmp(linea, command_list[i]) == 0) return i;
	return -1;
}
