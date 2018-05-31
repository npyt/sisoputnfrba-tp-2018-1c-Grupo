#include <libgrupo/headers.h>
#include "include/headers.h"

t_log * logger;
t_config * config;

t_list * instances = NULL;

DistributionAlgorithm DISTRIBUTION_ALG;
int eq_load_alg_last_used_inst = -1;

int main() {
	printf("COORDINATOR");

	logger = log_create("coordinator_logger.log", "COORDINATOR", true, LOG_LEVEL_TRACE);
	config = config_create("coordinator_config.cfg");

	instances = list_create();

	char * dist_al_temp = config_get_string_value(config, "DISTR_ALG");
	if( strcmp(dist_al_temp, "LSU")==0 ) {
		DISTRIBUTION_ALG = LSU;
	} else if( strcmp(dist_al_temp, "EL")==0 ) {
		DISTRIBUTION_ALG = EL;
	} else if( strcmp(dist_al_temp, "KE")==0 ) {
		DISTRIBUTION_ALG = KE;
	} else {
		log_error(logger, "[UNKNOWKN_DISTRIBUTION_ALG]");
		return -1;
	}
	free(dist_al_temp);

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

	pthread_t w_thread_id;
	pthread_create(&w_thread_id, NULL, w_thread, server_socket);

	pthread_exit(NULL);
	list_destroy(instances);
	close(server_socket);

	return 0;
}

void * w_thread(int a) {
	sleep(4);
	log_info(logger, "INSTANCE %d", get_instance_index_to_use());
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

			case ESI_COORD_HANDSHAKE:
				log_info(logger, "[INCOMING_CONNECTION_ESI]");
				send_only_header(client_socket, ESI_COORD_HANDSHAKE_OK);
				break;
			case INSTANCE_COORD_HANDSHAKE:
				{
					char * temp_str = malloc(header->size);
					recv(client_socket, temp_str, header->size, 0);

					InstanceRegistration * ir = malloc(sizeof(InstanceRegistration));
					ir->name = temp_str;
					ir->socket = client_socket;
					ir->status = AVAILABLE;
					list_add(instances, ir);

					log_info(logger, "[INCOMING_CONNECTION_INSTANCE][%s]", ir->name);

					InstanceInitConfig * instance_config = malloc(sizeof(InstanceInitConfig));
					instance_config->entry_count = atoi(config_get_string_value(config, "Q_ENTRIES"));
					instance_config->entry_size = atoi(config_get_string_value(config, "ENTRY_SIZE"));
					//TODO: Se crea la instancia y se inicia el thread que correponde.

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

int get_instance_index_to_use() {
	if(instances->elements_count == 0) {
		log_error(logger, "[NO_INSTANCES_IN_SYSTEM]");
		return -1;
	} else {

		switch (DISTRIBUTION_ALG) {
			case LSU:
				log_error(logger, "[LSU_ALG_NOT_YET_IMPLEMENTED]");
				return -1;
				break;
			case EL:
				{
					int index_to_use = eq_load_alg_last_used_inst + 1;
					int iterations = 0;
					if (index_to_use == instances->elements_count) { index_to_use = 0; }

					while( ((InstanceRegistration*)(list_get(instances, index_to_use)))->status != AVAILABLE &&
							iterations<instances->elements_count ) {
						iterations++;
						index_to_use++;
						if (index_to_use == instances->elements_count) { index_to_use = 0; }
					}
					if(((InstanceRegistration*)(list_get(instances, index_to_use)))->status == AVAILABLE) {
						return index_to_use;
					} else {
						log_error(logger, "[NO_AVAILABLE_INSTANCES_IN_SYSTEM]");
						return -1;
					}
				}
				break;
			case KE:
				log_error(logger, "[KE_ALG_NOT_YET_IMPLEMENTED]");
				return -1;
				break;
			default:
				log_error(logger, "[DIST_ALGORITH_UNKNOWN]");
				return -1;
				break;
		}
	}
}
