#include <libgrupo/headers.h>
#include <parsi/parser.h>

t_log * logger;
FILE * fp;

typedef struct {
	int planner_socket;
	int coord_socket;
} SocketToListen;

char esi_name[ESI_NAME_MAX_SIZE];

void * listening_threads(SocketToListen*);

int main(int argc, char **argv){
	// CONFIG
	fp = fopen("ESI_1", "r");
	if (fp == NULL){
		perror("Error al abrir el archivo: ");
		exit(EXIT_FAILURE);
	}

	t_config * config;
	logger = log_create("esi_logger.log", "ESI", true, LOG_LEVEL_TRACE);
	config = config_create("esi_config.cfg");
	// END CONFIG

	// COORD CONECTION
	int coordinator_socket = connect_with_server(config_get_string_value(config, "IP_COORD"),
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
	int planner_socket = connect_with_server(config_get_string_value(config, "IP_PLAN"),
			atoi(config_get_string_value(config, "PORT_PLAN")));
	if (planner_socket < 0){
		log_error(logger, " ERROR AL CONECTAR CON EL PLANIFICADOR");
		return -1;
	} else {
		log_info(logger, " CONECTADO EN: %d", planner_socket);
	}
	log_info(logger, "Envío saludo al planificador");
	send_only_header(planner_socket, ESI_PLANNER_HANDSHAKE);
	// END PLANNER CONNECTION

	// LISTENING THREAD
	SocketToListen * sockets_to_listen = malloc(sizeof(SocketToListen));
	sockets_to_listen->coord_socket = coordinator_socket;
	sockets_to_listen->planner_socket = planner_socket;
	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_threads, sockets_to_listen);
	// END LISTENING THREAD

	pthread_exit(NULL);
	fclose(fp);
    return EXIT_SUCCESS;
}

void parser(int coordinator_socket, int planner_socket){
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    InstructionDetail * id = malloc(sizeof(InstructionDetail));
    strcpy(id->ESIName, esi_name);

    if((read = getline(&line, &len, fp)) != -1) {
		t_esi_operacion parsed = parse(line);
		if(parsed.valido){

			switch(parsed.keyword){

				case GET:
					id->operation = GET_OP;
					strcpy(id->key, parsed.argumentos.GET.clave);

					log_info(logger, "OPERACION GET %s", id->key);

					send_content_with_header(coordinator_socket,
							INSTRUCTION_DETAIL_TO_COODRINATOR, id,
							sizeof(InstructionDetail));
					break;

				case SET:
					id->operation = SET_OP;
					strcpy(id->key, parsed.argumentos.SET.clave);
					id->opt_value = malloc(strlen(parsed.argumentos.SET.valor) * sizeof(char));
					strcpy(id->opt_value, parsed.argumentos.SET.valor);

					log_info(logger, "OPERACION SET %s %s", id->key, id->opt_value);

					send_content_with_header(coordinator_socket,
							INSTRUCTION_DETAIL_TO_COODRINATOR, id,
							sizeof(InstructionDetail));
					int aux_len = strlen(parsed.argumentos.SET.valor);
					send(coordinator_socket, &aux_len, sizeof(int), 0);
					send(coordinator_socket, id->opt_value, strlen(id->opt_value) * sizeof(char), 0);
					break;

				case STORE:
					id->operation = STORE_OP;
					strcpy(id->key, parsed.argumentos.STORE.clave);

					log_info(logger, "OPERACION STORE %s", id->key);

					send_content_with_header(coordinator_socket,
							INSTRUCTION_DETAIL_TO_COODRINATOR, id,
							sizeof(InstructionDetail));
					break;

				default:
					log_error(logger, " ERROR AL PARSEAR LA LINEA <%s>. ABORTANDO", line);
					send_only_header(planner_socket, ESI_EXECUTION_FINISHED);
					exit(EXIT_FAILURE);
	            }


	        } else {
	        	log_error(logger, " LA LINEA <%s> NO ES VALIDA. ABORTANDO", line);
	        	send_only_header(planner_socket, ESI_EXECUTION_FINISHED);
	            exit(EXIT_FAILURE);
	        }
	    }
	if(feof(fp)) {
		//EOF
		log_info(logger, "ESI FINALIZADO");
		send_only_header(planner_socket, ESI_EXECUTION_FINISHED);
		close(planner_socket);
		close(coordinator_socket);
		if (line) free(line);
		free(id);
		exit(EXIT_SUCCESS);
	}
}

void * listening_threads(SocketToListen * socket_to_listen){
	int coordinator_socket = socket_to_listen->coord_socket;
	int planner_socket = socket_to_listen->planner_socket;
	fd_set master;
	FD_ZERO(&master);
	FD_SET(coordinator_socket, &master);
	FD_SET(planner_socket, &master);
	struct timeval tv;
	int returning;
	while(1){
		returning = select(coordinator_socket+planner_socket+1, &master, NULL, NULL, NULL);
		if(returning < 0){
			log_error(logger, " ERRORRRR");
			return -1;
		}else{
		if(FD_ISSET(coordinator_socket, &master)){
			MessageHeader * header = malloc(sizeof(MessageHeader));
			recv(coordinator_socket, header, sizeof(MessageHeader), 0);
			switch((*header).type){
				case ESI_COORD_HANDSHAKE_OK:
					log_info(logger, "El COORDINADOR aceptó mi conexión");
					fflush(stdout);
					break;
				case UNKNOWN_MSG_TYPE:
					log_error(logger, "[MY_MESSAGE_HASNT_BEEN_DECODED]");
					fflush(stdout);
					break;
				case OPERATION_ERROR:
					log_info(logger, "Me informan de ERROR");
					send_only_header(coordinator_socket, ESI_EXECUTION_FINISHED);
					// Terminar ejecución
					fflush(stdout);
					break;
				default:
					log_error(logger, "[UNKOWN_MESSAGE_RECIEVED]");
					send_only_header(coordinator_socket, UNKNOWN_MSG_TYPE);
					fflush(stdout);
					break;
			}
		}
		if(FD_ISSET(planner_socket, &master)){
			MessageHeader * header = malloc(sizeof(MessageHeader));
			recv(planner_socket, header, sizeof(MessageHeader), 0);
			switch((*header).type) {
				case PLANNER_ESI_RUN:
					log_info(logger, "[RUNNING_OK__PARSING_NEW_SENTENCE");
					parser(coordinator_socket, planner_socket);
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
					strcpy(esi_name, esi_buffer_name);
					fflush(stdout);
					break;
				case UNKNOWN_MSG_TYPE:
					log_error(logger, "[MY_MESSAGE_HASNT_BEEN_DECODED]");
					fflush(stdout);
					break;
				default:
					log_error(logger, "[UNKOWN_MESSAGE_RECIEVED]");
					send_only_header(planner_socket, UNKNOWN_MSG_TYPE);
					fflush(stdout);
					break;
			}
		}
		}
	}
}
