#include <libgrupo/headers.h>
#include "include/headers.h"

t_log * logger;
t_config * config;

t_list * instances = NULL;

DistributionAlgorithm DISTRIBUTION_ALG;
int eq_load_alg_last_used_inst = -1;

int planner_socket = -1;

void * thread_listen_esi(int esi_socket);

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
				planner_socket = client_socket;
				break;
			case ESI_COORD_HANDSHAKE:
				log_info(logger, "[INCOMING_CONNECTION_ESI]");

				pthread_t e_thread_id;
				pthread_create(&e_thread_id, NULL, thread_listen_esi, client_socket);
				pthread_exit(NULL);

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
		free(header);
	}
}

void * thread_listen_esi(int esi_socket) {
	send_only_header(esi_socket, ESI_COORD_HANDSHAKE_OK);
	log_info(logger, "[STARTED_LISTENING_THREAD_FOR_ESI]");

	while(1) {
		MessageHeader * header = malloc(sizeof(MessageHeader));
		int rec = recv(esi_socket, header, sizeof(MessageHeader), 0);

		if(rec != -1) {
			switch((*header).type) {
				case TEST_SEND:
					log_info(logger, "ATENCIONTEST");
					break;
				case INSTRUCTION_DETAIL_TO_COODRINATOR:
					log_info(logger, "[INCOMING_OPERATION_FROM_ESI]");

					InstructionDetail * id = malloc(sizeof(InstructionDetail));
					recv(esi_socket, id, sizeof(InstructionDetail), 0);
					if(id->operation == SET_OP) {
						int value_size;
						recv(esi_socket, &value_size, sizeof(int), 0);
						id->opt_value = malloc(sizeof(char) * value_size);
						recv(esi_socket, id->opt_value, sizeof(char) * value_size, 0);
						log_info(logger, "OP_REC_SET_%s_%s_%s", id->ESIName, id->key, id->opt_value);
					} else if (id->operation == GET_OP) {
						log_info(logger, "OP_REC_GET_%s_%s", id->ESIName, id->key);
					} else if(id->operation == STORE_OP){
						log_info(logger, "OP_REC_STORE_%s_%s", id->ESIName, id->key);
					}

					CoordinatorPlannerCheck * check = malloc(sizeof(CoordinatorPlannerCheck));
					strcpy(check->ESIName, id->ESIName);
					strcpy(check->key, id->key);
					check->operation = id->operation;

					log_info(logger, "[CHECKING_OPERATION_PERMISSIONS]");
					switch(id->operation) {
						case GET_OP:
							send_content_with_header(planner_socket, CAN_ESI_GET_KEY, check, sizeof(CoordinatorPlannerCheck));
							break;
						case SET_OP:
							send_content_with_header(planner_socket, CAN_ESI_SET_KEY, check, sizeof(CoordinatorPlannerCheck));
							break;
						case STORE_OP:
							send_content_with_header(planner_socket, CAN_ESI_STORE_KEY, check, sizeof(CoordinatorPlannerCheck));
							break;
					}

					free(check);

					MessageHeader * header_check = malloc(sizeof(MessageHeader));
					recv(planner_socket, header_check, sizeof(MessageHeader), 0);

					switch(header_check->type) {
						case PLANNER_COORDINATOR_OP_OK:
							log_info(logger, "[PLANNER_OK][LOOKING_FOR_AVAILABLE_INSTANCE]");
							InstanceRegistration * target_instance = list_get(instances, get_instance_index_to_use());
							log_info(logger, "[INSTANCE_CHOSEN_TO_EXECUTE][%s]", target_instance->name);
							send_content_with_header(target_instance->socket, INSTRUCTION_DETAIL_TO_INSTANCE, id, sizeof(InstructionDetail));
							if(id->operation == SET_OP) {
								send(target_instance->socket, strlen(id->opt_value), sizeof(int), 0);
								send(target_instance->socket, id->opt_value, strlen(id->opt_value), 0);
							}
							log_info(logger, "[INSTRUCTION_SENT_TO_INSTANCE][AWAITING_CONFIRMATION]");

							header_check = malloc(sizeof(MessageHeader));
							recv(target_instance->socket, header_check, sizeof(MessageHeader), 0);
							switch((header_check)->type) {
								case INSTANCE_REPORTS_SUCCESSFUL_OP:
									log_info(logger, "[OPERATION_SUCCESSFUL]");
									ResourceAllocation * allocation_change = malloc(sizeof(ResourceAllocation));

									strcpy(allocation_change->ESIName, id->ESIName);
									strcpy(allocation_change->key, id->key);
									switch(id->operation) {
										case GET_OP:
											allocation_change->status = BLOCKED;
											break;
										case SET_OP:
											allocation_change->status = BLOCKED;
											break;
										case STORE_OP:
											allocation_change->status = RELEASED;
											break;
									}

									//Aviso operación a Planner
									send_content_with_header(planner_socket, RESOURCE_STATUS_CHANGE_TO_PLANNER, allocation_change, sizeof(ResourceAllocation));
									send_only_header(esi_socket, COORD_ESI_EXECUTED);
									free(allocation_change);
									break;
								case INSTANCE_REPORTS_FAILED_OP:
								default:
									log_info(logger, "[OPERATION_FAILED]");
									break;
							}

							free(target_instance);
							break;
						case PLANNER_COORDINATOR_OP_FAILED:
							log_error(logger, "[PLANNER_DIDNT_AUTHORIZE_OPERATION]");
							break;
					}

					free(header_check);
					free(id);
					break;
				case UNKNOWN_MSG_TYPE:
					log_error(logger, "[MY_MESSAGE_HASNT_BEEN_DECODED]");
					break;
				default:
					log_info(logger, "[UNKOWN_MESSAGE_RECIEVED]");
					send_only_header(esi_socket, UNKNOWN_MSG_TYPE);
					break;
			}
			free(header);
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
