#include "libgrupo/headers.h"
#include <parsi/parser.h>

t_log * logger;

//typedef struct readThreadParams {
//	int planner_socket;
//	int coord_socket;
//} ThreadParams;

int main(int argc, char **argv){
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;


	t_config * config;

	logger = log_create("esi_logger.log", "ESI", true, LOG_LEVEL_TRACE);
	config = config_create("esi_config.cfg");
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
		log_info(logger, " CONECTADO EN: %d",planner_socket);
	}
	log_info(logger, "Envío saludo al planificador");
	send_only_header(planner_socket, ESI_PLANNER_HANDSHAKE);
	// END PLANNER CONNECTION

	// LISTENING THREAD
	//	ThreadParams readParams;
	//	readParams.coord_socket=coordinator_socket;
	//	readParams.planner_socket=planner_socket;

	pthread_t listening_thread_planner;
	pthread_create(&listening_thread_planner, NULL, listening_thread, planner_socket);

	pthread_t listening_thread_coordinator;
	pthread_create(&listening_thread_coordinator, NULL, listening_thread, coordinator_socket);

	// END LISTENING THREAD
//
//    fp = fopen(argv[1], "r");
//    if (fp == NULL){
//        perror("Error al abrir el archivo: ");
//        exit(EXIT_FAILURE);
//    }
//
//    while ((read = getline(&line, &len, fp)) != -1) {
//    	t_esi_operacion parsed = parse(line);
//
//        if(parsed.valido){
//            switch(parsed.keyword){
//                case GET:
//                    printf("GET\tclave: <%s>\n", parsed.argumentos.GET.clave);
//                    break;
//                case SET:
//                    printf("SET\tclave: <%s>\tvalor: <%s>\n", parsed.argumentos.SET.clave, parsed.argumentos.SET.valor);
//                    break;
//                case STORE:
//                    printf("STORE\tclave: <%s>\n", parsed.argumentos.STORE.clave);
//                    break;
//
//                default:
//                    fprintf(stderr, "No pude interpretar <%s>\n", line);
//                    exit(EXIT_FAILURE);
//            }
//
//            destruir_operacion(parsed);
//        } else {
//            fprintf(stderr, "La linea <%s> no es valida\n", line);
//            exit(EXIT_FAILURE);
//        }
//    }
//
//    fclose(fp);
//    if (line)
//        free(line);

	pthread_exit(NULL);
    return EXIT_SUCCESS;
}

void * listening_thread(int server_socket) {

	while(1) {
		MessageHeader * header = malloc(sizeof(MessageHeader));
		int rec = recv(server_socket, header, sizeof(MessageHeader), 0);
		//Procesar el resto del mensaje dependiendo del tipo recibido
		switch((*header).type) {
			case PLANNER_ESI_RUN:
				log_info(logger, "El PLANNER me pide parsear la primer línea");
				// mensaje = parsearlinea()
				// send mensaje a coordinador
				fflush(stdout);
				break;
			case OPERATION_ERROR:
				log_info(logger, "Me informan de ERROR");
				// mensaje a planificador (OPERATION_ERROR)
				// terminar ejecución
				fflush(stdout);
				break;
			case ESI_COORD_HANDSHAKE_OK:
				log_info(logger, "El COORDINADOR aceptó mi conexión");
				fflush(stdout);
				break;
			case ESI_PLANNER_HANDSHAKE_OK:
				log_info(logger, "El PLANIFICADOR aceptó mi conexión");
				// recv(planner_socket, header, sizeof(MessageHeader), 0) == -1)
				// log_info(logger, "Mi ID de ESI es : %d", id_esi);
				fflush(stdout);
				break;
			case UNKNOWN_MSG_TYPE:
				log_error(logger, "[MY_MESSAGE_HASNT_BEEN_DECODED]");
				break;
			default:
				log_error(logger, "[UNKOWN_MESSAGE_RECIEVED]");
				send_only_header(server_socket, UNKNOWN_MSG_TYPE);
				break;
		}
	}
}
