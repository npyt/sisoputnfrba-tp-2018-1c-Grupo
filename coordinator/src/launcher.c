#include <libgrupo/headers.h>

t_log * logger;
t_config * config;

int main() {
	printf("COORDINATOR");

	logger = log_create("coordinator_logger.log", "COORDINATOR", true, LOG_LEVEL_TRACE);
	config = config_create("coordinator_config.cfg");

	t_list * instances = list_create();

	int server_socket = server_start(atoi(config_get_string_value(config, "PORT")));
	if (server_socket == -1) {
		log_error(logger, "ERROR AL INICIAR EL SERVIDOR");
		return 1;
	} else {
		log_info(logger, "SERVIDOR INICIADO");
	}

	if (listen(server_socket, 5) == -1) {
		log_error(logger, "ERROR AL ESCUCHAR EN PUERTO");
	} else {
		log_info(logger, "SERVIDOR ESCUCHANDO %d", atoi(config_get_string_value(config, "PORT")));
	}


	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, server_socket);

	pthread_exit(NULL);
	list_destroy(instances);
	close(server_socket);

	return 0;
}

void * listening_thread(int server_socket) {
	while(1) {
		struct sockaddr_in client;
		int c = sizeof(struct sockaddr_in);
		int client_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t *)&c);
		log_info(logger, "CONEXION RECIBIDA EN SOCKET %d", client_socket);

		MessageHeader * header = malloc(sizeof(MessageHeader));

		int rec = recv(client_socket, header, sizeof(MessageHeader), 0);

		//Procesar el resto del mensaje dependiendo del tipo recibido
		switch((*header).type) {
			case PLANNER_COORD_HANDSHAKE:
				log_info(logger, "[INCOMING_CONNECTION_PLANNER]");
				send_only_header(client_socket, PLANNER_COORD_HANDSHAKE_OK);
				break;
			case INSTANCE_COORD_HANDSHAKE:
				log_info(logger, "[INCOMING_CONNECTION_INSTANCE]");
				{
					InstanceInitConfig * instance_config = malloc(sizeof(InstanceInitConfig));
					instance_config->entry_count = atoi(config_get_string_value(config, "Q_ENTRIES"));
					instance_config->entry_size = atoi(config_get_string_value(config, "ENTRY_SIZE"));
					//Se crea la instancia y se inicia el thread que correponde. Además, informar por socket
					//los datos correpondientes (cantidad de entradas, tamaño de cada entrada, etc.)

					send_content_with_header(client_socket, INSTANCE_COORD_HANDSHAKE_OK, instance_config, sizeof(InstanceInitConfig));
					free(instance_config);
				}
				break;
			case UNKNOWN_MSG_TYPE:
				log_error(logger, "[MY_MESSAGE_HASNT_BEEN_DECODED]");
				break;
			default:
				log_info(logger, "[UNKOWN_MESSAGE_RECIEVED]");
				send_only_header(client_socket, UNKNOWN_MSG_TYPE);
				break;
		}
	}
}
