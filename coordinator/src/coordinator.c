#include <chucknorris/allheaders.h>

t_config * config;
t_log * logger;
t_list * instances;

int last_used_instance;

CoordinatorConfig settings;
t_list * resources;

void * listening_thread(int server_socket);
InstanceRegistration * get_instance_for_process(InstructionDetail * instruction);
InstanceRegistration * search_instance_by_name(char name[INSTANCE_NAME_MAX]);
void instance_limit_calculation();
int find_first_Lowercase (char *key);
int get_instance_index_by_alg(char *key);


int main(int argc, char **argv) {
	if(argv[1] == NULL) {
		//exit_with_message("Please write a config filename.", EXIT_FAILURE);
		argv[1] = malloc(sizeof(char) * 1024);
		strcpy(argv[1], "config.cfg");
	}

	config = config_create(argv[1]);
	logger = log_create("log.log", "COORDINATOR", false, LOG_LEVEL_TRACE);

	settings.port = config_get_int_value(config, "PORT");
	settings.entry_count = config_get_int_value(config, "ENTRY_COUNT");
	settings.entry_size = config_get_int_value(config, "ENTRY_SIZE");
	settings.delay = config_get_int_value(config, "DELAY");
	char * buffer = malloc(128 * sizeof(char));
	strcpy(buffer, config_get_string_value(config, "DIST_ALG"));
	if(strcmp(buffer, "LSU") == 0) {
		settings.dist_alg = LSU;
	} else if(strcmp(buffer, "EL") == 0) {
		settings.dist_alg = EL;
	} else if(strcmp(buffer, "KE") == 0) {
		settings.dist_alg = KE;
	}
	free(buffer);
	config_destroy(config);

	last_used_instance = -1;

	resources = list_create();

	int my_socket = prepare_socket_in_port(settings.port);
	if (my_socket == -1) {
		print_and_log_trace(logger, "[FAILED_TO_OPEN_COORDINATOR_PORT]");
		exit(EXIT_FAILURE);
	}
	instances = list_create();

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, my_socket);

	pthread_exit(NULL);

	return EXIT_SUCCESS;
}

void * listening_thread(int server_socket) {
	int clients[MAX_SERVER_CLIENTS];
	int a, max_sd, activity_socket, incoming_socket;
	fd_set master_set;
	for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
		clients[a] = 0;
	}
	clients[0] = server_socket;

	if(listen(server_socket, MAX_SERVER_CLIENTS) == -1) {
		print_and_log_trace(logger, "[FAILED_TO_OPEN_LISTEN_AT_PORT]");
		exit(EXIT_FAILURE);
	}

	print_and_log_trace(logger, "[READY_FOR_CONNECTIONS]");

	while(1) {
		FD_ZERO(&master_set);
		FD_SET(server_socket, &master_set);
		max_sd = server_socket;
		for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
			if(clients[a] > 0) {
				FD_SET(clients[a], &master_set);
				//print_and_log_trace(logger, "[SOCKET][%d]", clients[a]);
				if(max_sd < clients[a]) {
					max_sd = clients[a];
				}
			}
		}
		activity_socket = select(max_sd + 1, &master_set, NULL, NULL, NULL);
		//print_and_log_trace(logger, "[PELADO SAMPAOLI]");

		if(FD_ISSET(server_socket, &master_set)) {
			incoming_socket = server_socket;

			struct sockaddr_in client;
			int c = sizeof(struct sockaddr_in);
			incoming_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t *)&c);

			//From listen

			print_and_log_trace(logger, "[INCOMING_CONNECTION]", incoming_socket);

			for(a=0 ; a<MAX_SERVER_CLIENTS; a++) {
				if(clients[a] == 0) {
					clients[a] = incoming_socket;
					break;
				}
			}


		}
		for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
			if(FD_ISSET(clients[a], &master_set)) {

				incoming_socket = clients[a];
				//From another client
				//Recieve Header
				MessageHeader * i_header = malloc(sizeof(MessageHeader));
				if (recieve_header(incoming_socket, i_header) <= 0 ) {
					//Disconnected


					clients[a] = 0;
					print_and_log_trace(logger, "[SOCKET_DISCONNECTED]");
				} else {
					//New message

					MessageHeader * header = malloc(sizeof(MessageHeader));

					switch(i_header->type){
						case HSK_INST_COORD:
							print_and_log_trace(logger, "[NEW_INSTANCE]");
								InstanceRegistration * ir = malloc(sizeof(InstanceRegistration));
								ir->socket = incoming_socket;
								recieve_data(incoming_socket, ir->name, INSTANCE_NAME_MAX);

								InstanceData * data = malloc(sizeof(InstanceData));
								data->entry_count = settings.entry_count;
								data->entry_size = settings.entry_size;

								InstanceRegistration * prev_inst = search_instance_by_name(ir->name);
								if(prev_inst == NULL) {
									print_and_log_trace(logger, "[SAYS_NAME_IS][%s]", ir->name);
									ir->free_entries = settings.entry_count;

									list_add(instances, ir);

									header->type = HSK_INST_COORD_OK;
									send_header_and_data(incoming_socket, header, data, sizeof(InstanceData));
								} else {
									free(ir);
									print_and_log_trace(logger, "[WAS_AN_EXISTING_CONNECTION][%s][REPORTING_IT_MUST_RELOAD_KEYS]", prev_inst->name);
									prev_inst->socket = incoming_socket;

									header->type = HSK_INST_COORD_RELOAD;
									send_header_and_data(incoming_socket, header, data, sizeof(InstanceData));
								}
								instance_limit_calculation();//llamar asignaccion si es KE MEC
							break;
						case HSK_PLANNER_COORD:
								print_and_log_trace(logger, "[PLANNER_HANDSHAKE][SAVING_SOCKET]");

								settings.planner_socket = incoming_socket;

								header->type = HSK_PLANNER_COORD_OK;
								send_header(incoming_socket, header);
								break;
						case HSK_ESI_COORD:
							print_and_log_trace(logger, "[NEW_ESI_CONNECTION]");
							header->type = HSK_ESI_COORD_OK;
							send_header(incoming_socket, header);
							break;
						case INSTRUCTION_ESI_COORD:
							print_and_log_trace(logger, "[INCOMING_INSTRUCTION_FROM_ESI]");
							InstructionDetail * instruction = malloc(sizeof(InstructionDetail));
							recieve_data(incoming_socket, instruction, sizeof(InstructionDetail));

							print_and_log_trace(logger, "[ESI_ID][ESI_%d]", instruction->esi_id);
							print_instruction(instruction);

							print_and_log_trace(logger, "[DELAY]");
							sleep(settings.delay / 1000);

							send_message_type(settings.planner_socket, INSTRUCTION_PERMISSION);
							send_data(settings.planner_socket, instruction, sizeof(InstructionDetail));
							print_and_log_trace(logger, "[ASKING_FOR_PLANNER_PERMISSION]");

							//Waiting for response
							MessageHeader * response_header = malloc(sizeof(MessageHeader));
							recieve_header(settings.planner_socket, response_header);

							switch (response_header->type) {
								case INSTRUCTION_ALLOWED:
									print_and_log_trace(logger, "[PERMISSION_GRANTED]");

									InstanceRegistration * instance = get_instance_for_process(instruction);
									if (instance != NULL && instruction->type != GET_OP) { //Available instance and SET or STORE
										header->type = INSTRUCTION_COORD_INST;
										send_header_and_data(instance->socket, header, instruction, sizeof(InstructionDetail));
										print_and_log_trace(logger, "[INSTRUCTION_SENT_TO_INSTANCE][%s]", instance->name);

										//Waiting for response
										recieve_header(instance->socket, response_header);
										switch(response_header->type) {
											case INSTRUCTION_OK_TO_COORD:
												print_and_log_trace(logger, "[OPERATION_SUCCESSFUL][INFORMING_PLANNER]");
												InstanceConfig * receive_entries = malloc(sizeof(InstanceConfig));
												recieve_data(instance->socket, receive_entries->free_entries, sizeof(InstanceRegistration)); //receiving free_entries from instance
												instance->free_entries = receive_entries->free_entries;
												free(receive_entries);

												send_message_type(settings.planner_socket, INSTRUCTION_OK_TO_PLANNER);

												if(instruction->type != SET_OP) {
													print_and_log_trace(logger, "[NEW_RESOURCE_ALLOCATION][INFORMING_PLANNER]");
													ResourceAllocation * ra = malloc(sizeof(ResourceAllocation));
													ra->esi_id = instruction->esi_id;
													if(instruction->type == GET_OP) {
														ra->type = BLOCKED;
													} else if(instruction->type == STORE_OP) {
														ra->type = RELEASED;
													}
													strcpy(ra->key, instruction->key);
													header->type = NEW_RESOURCE_ALLOCATION;
													send_header_and_data(settings.planner_socket, header, ra, sizeof(ResourceAllocation));
													free(ra);
												}

												break;
											case INSTRUCTION_FAILED_TO_COORD:
												print_and_log_trace(logger, "[OPERATION_FAILED][INFORMING_ESI]");
												send_message_type(incoming_socket, INSTRUCTION_FAILED_TO_ESI);
												break;
										}

									} else if (instruction->type == GET_OP) { //GET OP, allocated instance but no need to send instruction
										print_and_log_trace(logger, "[OPERATION_SUCCESSFUL][INFORMING_PLANNER]");
										send_message_type(settings.planner_socket, INSTRUCTION_OK_TO_PLANNER);
										print_and_log_trace(logger, "[NEW_RESOURCE_ALLOCATION][INFORMING_PLANNER]");
										ResourceAllocation * ra = malloc(sizeof(ResourceAllocation));
										ra->esi_id = instruction->esi_id;
										ra->type = BLOCKED;
										strcpy(ra->key, instruction->key);
										header->type = NEW_RESOURCE_ALLOCATION;
										send_header_and_data(settings.planner_socket, header, ra, sizeof(ResourceAllocation));

										free(ra);
									} else { //Failed instruction : unknown key by coordinator
										print_and_log_trace(logger, "[OPERATION_FAILED][INFORMING_ESI]");
										send_message_type(incoming_socket, INSTRUCTION_FAILED_TO_ESI);
									}
									break;
								case INSTRUCTION_NOT_ALLOWED:
									print_and_log_trace(logger, "[OPERATION_FAILED][PROBABLE_ESI_BLOCK]");
									break;
								case INSTRUCTION_NOT_ALLOWED_AND_ABORT:
									print_and_log_trace(logger, "[OPERATION_FAILED][ESI_ABORTION_NOTICE]");
									send_message_type(incoming_socket, INSTRUCTION_FAILED_TO_ESI);
									break;
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

ResourceRegistration * search_resource(char key[KEY_NAME_MAX]) {
	int a;
	for(a=0 ; a<resources->elements_count ; a++) {
		ResourceRegistration * re = list_get(resources, a);
		if(strcmp(re->key, key) == 0) {
			return re;
		}
	}
	return NULL;
}

InstanceRegistration * search_instance_by_name(char name[INSTANCE_NAME_MAX]) {
	int a;
	for(a=0 ; a<instances->elements_count ; a++) {
		InstanceRegistration * re = list_get(instances, a);
		if(strcmp(re->name, name) == 0) {
			return re;
		}
	}
	return NULL;
}

InstanceRegistration * get_instance_for_process(InstructionDetail * instruction) {
	if(instruction->type == GET_OP) {
		ResourceRegistration * re = malloc(sizeof(ResourceRegistration));
		re->instance = NULL;
		strcpy(re->key, instruction->key);
		list_add(resources, re);
		return NULL;
	} else {
		ResourceRegistration * re = search_resource(instruction->key);
		if(re->instance == NULL) {
			re->instance = list_get(instances, get_instance_index_by_alg(instruction->key));
		}
		return re->instance;
	}
	return NULL;
}

int find_first_Lowercase (char *key){
    for (int i=0;i<(string_length(key)-1);i++){
        	int value = key[i];
        	if ((value >= 'a' ) && (value <= 'z')){
        		return value;
        	        }
    }
    return -1; //error?
}

void instance_limit_calculation(){
	int letters = 26;
	int inst_number = list_size(instances);
	int letters_per_instance = letters / inst_number;
	int first_letter = 'a';
	InstanceRegistration* inst;
	for(int i =0;i < inst_number;i++){
	//if (isup){ ----(later)
		inst= list_get(instances,i);
		inst->inf = first_letter;
		first_letter += letters_per_instance;
		inst->sup=first_letter;//first_letter ahora toma el valor de la ultima.
	/*}else{
	 * inst->inf=-1
	 * inst->sup=-1}*/
	}
	if(first_letter < 'z'){
		InstanceRegistration* last_inst = list_get(instances,list_size(instances)-1);
		last_inst->sup = 'z';
	}
}
bool max_free_entries_instance(InstanceRegistration *instance_1, InstanceRegistration * instance_2) {
	return (instance_1->free_entries > instance_2->free_entries);
}

int get_instance_index_by_alg(char *key) { //TODO algs
	int chosen_index = 0;
	int value = find_first_Lowercase (key);
	int inst_number = list_size(instances);
	InstanceRegistration* inst;
	switch(settings.dist_alg) {
		case LSU:
			list_sort(instances, (void *)max_free_entries_instance);
			chosen_index = 0;
			break;
		case EL:
			last_used_instance++;
			if(last_used_instance == instances->elements_count) {
				last_used_instance = 0;
			}
			chosen_index = last_used_instance;
			break;
		case KE:
			for(int index =0;index < inst_number;index++){
			inst= list_get(instances,index);
				if((value >= inst->inf) && (value <= inst->sup)){
					return index;
				}
			}
			break;
	}
	return chosen_index;
}
