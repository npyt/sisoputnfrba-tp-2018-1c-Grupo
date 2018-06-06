#include <libgrupo/headers.h>
#include <parsi/parser.h>

t_log * logger;
FILE * fp;

int coordinator_socket;
int planner_socket;

int main(int argc, char **argv){
	fp = fopen(argv[1], "r");
//	if (fp == NULL){
//		perror("Error al abrir el archivo: ");
//		exit(EXIT_FAILURE);
//	}


	t_config * config;

	logger = log_create("esi_logger.log", "ESI", true, LOG_LEVEL_TRACE);
	config = config_create("esi_config.cfg");
	// COORD CONECTION
	coordinator_socket = connect_with_server(config_get_string_value(config, "IP_COORD"),
			atoi(config_get_string_value(config, "PORT_COORD")));
	if (coordinator_socket < 0){
		log_error(logger, " ERROR AL CONECTAR CON EL COORDINADOR");
		return 1;
	} else {
		log_info(logger, " CONECTADO EN: %d", coordinator_socket);
	}
	log_info(logger, "Envío saludo al coordinador");
	send_only_header(coordinator_socket, ESI_COORD_HANDSHAKE);
	// END COORD CONNECTION

	// PLANNER CONNECTION
	planner_socket = connect_with_server(config_get_string_value(config, "IP_PLAN"),
			atoi(config_get_string_value(config, "PORT_PLAN")));
	if (planner_socket < 0){
		log_error(logger, " ERROR AL CONECTAR CON EL PLANIFICADOR");
		return -1;
	} else {
		log_info(logger, " CONECTADO EN: %d",planner_socket);
	}
	log_info(logger, "Envío saludo al planificador");
	send_only_header(planner_socket, ESI_PLANNER_HANDSHAKE);
	// END PLANNER CONNECTION

	// LISTENING THREAD


	pthread_t listening_thread_coordinator;
	pthread_create(&listening_thread_coordinator, NULL, coordinator_listening_thread);

	pthread_t listening_thread_planner;
	pthread_create(&listening_thread_planner, NULL, planner_listening_thread);


	// END LISTENING THREAD

	fclose(fp);
    return EXIT_SUCCESS;
}

void parser(int coordinator_socket){
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    InstructionDetail * id = malloc(sizeof(InstructionDetail));

    if((read = getline(&line, &len, fp)) != -1) {
		t_esi_operacion parsed = parse(line);
		if(parsed.valido){

			switch(parsed.keyword){

				case GET:
					id->operation = GET_OP;
					strcpy(id->key, parsed.argumentos.GET.clave);

					send_content_with_header(coordinator_socket,
							INSTRUCTION_DETAIL_TO_COORDINATOR, id,
							sizeof(InstructionDetail));
					break;

				case SET:
					id->operation = SET_OP;
					strcpy(id->key, parsed.argumentos.SET.clave);
					strcpy(id->opt_value, parsed.argumentos.SET.valor);

					send_content_with_header(coordinator_socket,
							INSTRUCTION_DETAIL_TO_COORDINATOR, id,
							sizeof(InstructionDetail));
					send(coordinator_socket, strlen(id->opt_value), sizeof(int), 0);
					send(coordinator_socket, id->opt_value, strlen(id->opt_value), 0);
					break;

				case STORE:
					id->operation = STORE_OP;
					strcpy(id->key, parsed.argumentos.STORE.clave);

					send_content_with_header(coordinator_socket,
							INSTRUCTION_DETAIL_TO_COORDINATOR, id,
							sizeof(InstructionDetail));
					break;

	                default:
	                    log_error(logger, " ERROR AL PARSEAR LA LINEA <%s>. ABORTANDO", line);
	                    send_only_header(planner_socket, ESI_EXECUTION_FINISHED);
	                    exit(EXIT_FAILURE);
	            }

	            destruir_operacion(parsed);
	        } else {
	        	log_error(logger, " LA LINEA <%s> NO ES VALIDA. ABORTANDO", line);
	        	send_only_header(planner_socket, ESI_EXECUTION_FINISHED);
	            exit(EXIT_FAILURE);
	        }
	    }

		//EOF
		send_only_header(planner_socket, ESI_EXECUTION_FINISHED);
	    if (line) free(line);
	    free(id);
		exit(EXIT_SUCCESS);
}

void * coordinator_listening_thread() {

	while(1) {
		MessageHeader * header = malloc(sizeof(MessageHeader));
		int rec = recv(coordinator_socket, header, sizeof(MessageHeader), 0);
		//Procesar el resto del mensaje dependiendo del tipo recibido
		switch((*header).type) {
			case COORD_ESI_EXECUTED:
				send_only_header(planner_socket, ESI_EXECUTION_LINE_OK);
				break;
			case OPERATION_ERROR:
				log_info(logger, "El COORDINADOR me informa de ERROR");
				send_only_header(planner_socket, ESI_EXECUTION_FINISHED);
				fflush(stdout);
				exit(EXIT_FAILURE);
				break;
			case ESI_COORD_HANDSHAKE_OK:
				log_info(logger, "El COORDINADOR aceptó mi conexión");
				fflush(stdout);
				break;
			case UNKNOWN_MSG_TYPE:
				log_error(logger, "[MY_MESSAGE_HASNT_BEEN_DECODED]");
				break;
			default:
				log_error(logger, "[UNKNOWN_MESSAGE_RECIEVED]");
				send_only_header(coordinator_socket, UNKNOWN_MSG_TYPE);
				break;
		}
		free(header);
	}
}

void * planner_listening_thread() {

	while(1) {
		MessageHeader * header = malloc(sizeof(MessageHeader));
		int rec = recv(planner_socket, header, sizeof(MessageHeader), 0);
		//Procesar el resto del mensaje dependiendo del tipo recibido
		switch((*header).type) {
			case PLANNER_ESI_RUN:
				log_info(logger, "[RUNNING_OK__PARSING_NEW_SENTENCE");
				parser(coordinator_socket);
				fflush(stdout);
				break;
			case ESI_PLANNER_HANDSHAKE_OK:
				log_info(logger, "El PLANIFICADOR aceptó mi conexión");

				// ========== RECV STRUCTURE REGISTRATION VARIATION ==========
				//ESIRegistration * esi_name = malloc(sizeof(ESIRegistration));
				//recv(server_socket, esi_name, sizeof(ESIRegistration), 0);
				//log_info(logger, "[REGISTERED_AS_%s]", esi_name->id);
				// ========== END RECV STRUCTURE REGISTRATION VARIATION ==========

				char * esi_buffer_name[ESI_NAME_MAX_SIZE];
				recv(planner_socket, esi_buffer_name, sizeof(ESI_NAME_MAX_SIZE), 0);
				log_info(logger, "[REGISTERED_AS_%s]", esi_buffer_name);
				fflush(stdout);
				break;
			case UNKNOWN_MSG_TYPE:
				log_error(logger, "[MY_MESSAGE_HASNT_BEEN_DECODED]");
				break;
			default:
				log_error(logger, "[UNKNOWN_MESSAGE_RECIEVED]");
				send_only_header(coordinator_socket, UNKNOWN_MSG_TYPE);
				break;
		}
		free(header);
	}
}
