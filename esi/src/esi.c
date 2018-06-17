#include <chucknorris/allheaders.h>
#include <parsi/parser.h>

t_config * config;
t_log * logger;

ESIConfig settings;

FILE * script_file;
int bytes_to_last_instruction;
int finished = 0;

InstructionDetail instruction;

void * listening_thread();

int main(int argc, char **argv) {
	if(argv[1] == NULL) {
		//exit_with_message("No especificó el script ESI.", EXIT_FAILURE);
		argv[1] = malloc(sizeof(char) * 1024);
		strcpy(argv[1], "ESI_Largo");
	}

	config = config_create("config.cfg");
	logger = log_create("log.log", "ESI", false, LOG_LEVEL_TRACE);

	script_file = fopen(argv[1], "r");
	bytes_to_last_instruction = 0;

	strcpy(settings.coord_ip, config_get_string_value(config, "COORD_IP"));
	settings.coord_port = config_get_int_value(config, "COORD_PORT");
	strcpy(settings.planner_ip, config_get_string_value(config, "PLANNER_IP"));
	settings.planner_port = config_get_int_value(config, "PLANNER_PORT");
	config_destroy(config);

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, NULL);

	pthread_exit(NULL);

	return EXIT_SUCCESS;
}

void parse_next_instruction() {
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	t_esi_operacion parsi_instruction;

	bytes_to_last_instruction = ftell(script_file);
	if(getline(&line, &len, script_file) != -1) {
		parsi_instruction = parse(line);

		instruction.esi_id = settings.id;
		switch(parsi_instruction.keyword) {
			case GET:
				instruction.type = GET_OP;
				strcpy(instruction.key, parsi_instruction.argumentos.GET.clave);
				break;
			case SET:
				instruction.type = SET_OP;
				strcpy(instruction.key, parsi_instruction.argumentos.SET.clave);
				strcpy(instruction.opt_value, parsi_instruction.argumentos.SET.valor);
				break;
			case STORE:
				instruction.type = STORE_OP;
				strcpy(instruction.key, parsi_instruction.argumentos.STORE.clave);
				break;
		}
		destruir_operacion(parsi_instruction);
	}
	if (line)
        free(line);
	if(feof(script_file)) {
		fclose(script_file);
		send_message_type(settings.planner_socket, ESI_FINISHED);
		send_data(settings.planner_socket, &settings.id, sizeof(int));
		finished = 1;
		print_and_log_trace(logger, "[END_EXECUTION]");
	}
}

void * listening_thread() {
	int clients[MAX_SERVER_CLIENTS];
	int a, max_sd, activity_socket, incoming_socket;
	fd_set master_set;

	settings.coordinator_socket = connect_with_server(settings.coord_ip, settings.coord_port);
	settings.planner_socket = connect_with_server(settings.planner_ip, settings.planner_port);

	for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
		clients[a] = 0;
	}
	clients[0] = settings.coordinator_socket;
	clients[1] = settings.planner_socket;

	send_message_type(settings.coordinator_socket, HSK_ESI_COORD);
	print_and_log_trace(logger, "[SAYING_HI_TO_COORDINATOR]");

	while(1) {
		FD_ZERO(&master_set);
		max_sd = -1;
		if (finished) {
			clients[0] = 0;
			clients[1] = 0;
		}
		for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
			if(clients[a] > 0) {
				FD_SET(clients[a], &master_set);
				if(max_sd < clients[a]) {
					max_sd = clients[a];
				}
			}
		}

		activity_socket = select(max_sd + 1, &master_set, NULL, NULL, NULL);

		for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
			if(FD_ISSET(clients[a], &master_set)) {
				incoming_socket = clients[a];
				//From another client
				//Recieve Header
				MessageHeader * i_header = malloc(sizeof(MessageHeader));
				if (recieve_header(incoming_socket, i_header) <= 0 ) {
					//Disconnected


					close(clients[a]);
					print_and_log_trace(logger, "[SOCKET_DISCONNECTED]");
				} else {
					//New message


					MessageHeader * header = malloc(sizeof(MessageHeader));

					switch(i_header->type) {
						case HSK_ESI_COORD_OK:
							print_and_log_trace(logger, "[COORDINATOR_SAYS_HI]");

							send_message_type(settings.planner_socket, HSK_ESI_PLANNER);
							print_and_log_trace(logger, "[SAYING_HI_TO_PLANNER]");
							break;
						case HSK_ESI_PLANNER_OK:
							recieve_data(incoming_socket, &settings.id, sizeof(int));
							print_and_log_trace(logger, "[PLANNER_SAYS_HI]");
							print_and_log_trace(logger, "[REPORTING_AS_ESI_%d]", settings.id);
							break;
						case INSTRUCTION_FAILED_TO_ESI:
							print_and_log_trace(logger, "[COORDINATOR_COMMANDS_ME_TO_ABORT]");
							finished = 1;
							send_message_type(settings.planner_socket, ESI_FINISHED);
							break;
						case EXECUTE_NEXT_INSTRUCTION:
							parse_next_instruction();

							if (!finished) {
								header->type = INSTRUCTION_ESI_COORD;
								send_header_and_data(settings.coordinator_socket, header, &instruction, sizeof(InstructionDetail));

								print_and_log_trace(logger, "[INSTRUCTION_SENT_TO_COORDINATOR]");
								print_instruction(&instruction);
							}
							break;
						case EXECUTE_PREV_INSTRUCTION:
							if (!finished) {
								header->type = INSTRUCTION_ESI_COORD;
								send_header_and_data(settings.coordinator_socket, header, &instruction, sizeof(InstructionDetail));

								print_and_log_trace(logger, "[INSTRUCTION_SENT_TO_COORDINATOR]");
								print_instruction(&instruction);
							}
							break;
					}

					free(header);

				}
				free(i_header);
			}
		}
	}
}
