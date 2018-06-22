#include "utils.h"

void print_instruction(InstructionDetail * instruction) {
	switch(instruction->type) {
		case GET_OP:
			printf("Instrucción GET sobre %s ESI %d\n", instruction->key, instruction->esi_id);
			fflush(stdout);
			break;
		case SET_OP:
			printf("Instrucción SET sobre %s ESI %d valor %s\n", instruction->key, instruction->esi_id, instruction->opt_value);
			fflush(stdout);
			break;
		case STORE_OP:
			printf("Instrucción STORE sobre %s ESI %d\n", instruction->key, instruction->esi_id);
			fflush(stdout);
			break;
	}
}

void exit_with_message(char * message, int exit_code) {
	printf("%s", message);
	exit(exit_code);
}

void print_and_log_trace(t_log * logger, char * template, ... ) {
	char * message;
	va_list arguments;
	va_start(arguments, template);

	message = string_from_vformat(template, arguments);
	printf(message);
	printf("\n");
	fflush(stdout);
	log_trace(logger, message);

	va_end(arguments);
}

void print_and_log_debug(t_log * logger, char * template, ... ) {
	char * message;
	va_list arguments;
	va_start(arguments, template);

	message = string_from_vformat(template, arguments);
	printf(message);
	printf("\n");
	fflush(stdout);
	log_debug(logger, message);

	va_end(arguments);
}

void print_and_log_info(t_log * logger, char * template, ... ) {
	char * message;
	va_list arguments;
	va_start(arguments, template);

	message = string_from_vformat(template, arguments);
	printf(message);
	printf("\n");
	fflush(stdout);
	log_info(logger, message);

	va_end(arguments);
}

void print_and_log_warning(t_log * logger, char * template, ... ) {
	char * message;
	va_list arguments;
	va_start(arguments, template);

	message = string_from_vformat(template, arguments);
	printf(message);
	printf("\n");
	fflush(stdout);
	log_warning(logger, message);

	va_end(arguments);
}

void print_and_log_error(t_log * logger, char * template, ... ) {
	char * message;
	va_list arguments;
	va_start(arguments, template);

	message = string_from_vformat(template, arguments);
	printf(message);
	printf("\n");
	fflush(stdout);
	log_error(logger, message);

	va_end(arguments);
}
