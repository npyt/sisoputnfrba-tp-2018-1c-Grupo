#include <chucknorris/allheaders.h>

//Init
t_config * config;
t_log * logger;
t_log * operation_logger;
t_list * instances;
t_list * resources;
CoordinatorConfig settings;

//Global
int last_used_instance;

void * listening_thread(int server_socket);
void * instance_thread(InstanceRegistration * ir);
void * planner_thread();
void * esi_thread(int incoming_socket);
bool is_up(InstanceRegistration * instance);
t_list * instance_resources(InstanceRegistration * instance);
InstanceRegistration * get_instance_for_process(InstructionDetail * instruction);
InstanceRegistration * search_instance_by_name(char name[INSTANCE_NAME_MAX]);
void instance_limit_calculation();
int find_first_lowercase (char *key);
int get_instance_index_by_alg(char* key, int simulation_mode);
char * get_instance_name_by_alg(char* key, int simulation_mode);
ResourceRegistration * search_resource(char key[KEY_NAME_MAX]);

pthread_mutex_t instance_feedback;

int pending_compacts = 0;

int main(int argc, char **argv) {
	if(argv[1] == NULL) {
		//exit_with_message("Please write a config filename.", EXIT_FAILURE);
		argv[1] = malloc(sizeof(char) * 1024);
		strcpy(argv[1], "config.cfg");
	}

	// Creating log and config files
	config = config_create(argv[1]);
	logger = log_create("log.log", "COORDINATOR", false, LOG_LEVEL_TRACE);
	operation_logger = log_create("operations.log", "COORDINATOR_OPERATIONS", false, LOG_LEVEL_TRACE);

	// Reading config file
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

	// First counters and list inits
	last_used_instance = -1;
	resources = list_create();
	instances = list_create();

	pthread_mutex_init(&instance_feedback);

	// Starting server socket
	int my_socket = prepare_socket_in_port(settings.port);
	if (my_socket == -1) {
		print_and_log_trace(logger, "[FAILED_TO_OPEN_COORDINATOR_PORT]");
		exit(EXIT_FAILURE);
	}

	// Threads
	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, my_socket);

	// Exit
	pthread_exit(NULL);
	return EXIT_SUCCESS;
}

void * listening_thread(int server_socket) {
	if(listen(server_socket, MAX_SERVER_CLIENTS) == -1) {
		print_and_log_trace(logger, "[FAILED_TO_OPEN_LISTEN_AT_PORT]");
		exit(EXIT_FAILURE);
	}

	print_and_log_trace(logger, "[READY_FOR_CONNECTIONS]");

	while(1) {
		struct sockaddr_in client;
		int c = sizeof(struct sockaddr_in);
		int incoming_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t *)&c);

		print_and_log_trace(logger, "[INCOMING_CONNECTION]", incoming_socket);

		//Recieve Header
		MessageHeader * i_header = malloc(sizeof(MessageHeader));
		if (recieve_header(incoming_socket, i_header) <= 0 ) {
			//Disconnected
			print_and_log_trace(logger, "[SOCKET_DISCONNECTED]");
		} else {
			//New message
			MessageHeader * header = malloc(sizeof(MessageHeader));

			switch(i_header->type){
				case HSK_INST_COORD:
					// Registrate Instance
					print_and_log_trace(logger, "[NEW_INSTANCE]");
					InstanceRegistration * ir = malloc(sizeof(InstanceRegistration));
					ir->socket = incoming_socket;
					recieve_data(incoming_socket, ir->name, INSTANCE_NAME_MAX);

					// Registrate Instance entries info
					InstanceData * data = malloc(sizeof(InstanceData));
					data->entry_count = settings.entry_count;
					data->entry_size = settings.entry_size;

					// Response OK
					header->type = HSK_INST_COORD_OK;
					send_header_and_data(ir->socket, header, data, sizeof(InstanceData));

					free(data);

					InstanceRegistration * prev_inst = search_instance_by_name(ir->name);
					if(prev_inst == NULL) {
						// Registrate NEW Instance
						print_and_log_trace(logger, "[SAYS_NAME_IS][%s]", ir->name);
						ir->free_entries = settings.entry_count;
						list_add(instances, ir);

						pthread_t instance_thread_id;
						pthread_create(&instance_thread_id, NULL, instance_thread, ir);
					} else {
						// Instance already exists
						print_and_log_trace(logger, "[WAS_AN_EXISTING_CONNECTION][%s][REPORTING_IT_MUST_RELOAD_KEYS]", prev_inst->name);
						prev_inst->socket = ir->socket;
						free(ir);

						// Registrate Instance entries info
						InstanceData * data = malloc(sizeof(InstanceData));
						data->entry_count = settings.entry_count;
						data->entry_size = settings.entry_size;

						header->type = HSK_INST_COORD_RELOAD;
						send_header_and_data(prev_inst->socket, header, data, sizeof(InstanceData));

						free(data);

						t_list * re_resources = instance_resources(prev_inst);
						print_and_log_info(logger, "Must Relocate %d resources", re_resources->elements_count);
						send_data(prev_inst->socket, &re_resources->elements_count, sizeof(int));

						for(int a=0 ; a<re_resources->elements_count ; a++) {
							ResourceRegistration * rr = list_get(re_resources, a);
							send_data(prev_inst->socket, rr->key, sizeof(char) * KEY_NAME_MAX);
						}
						prev_inst->isup = 1;
					}

					break;
				case HSK_PLANNER_COORD:
					print_and_log_trace(logger, "[PLANNER_HANDSHAKE][SAVING_SOCKET]");

					settings.planner_socket = incoming_socket;

					pthread_t planner_thread_id;
					pthread_create(&planner_thread_id, NULL, planner_thread, NULL);
					break;
				case HSK_ESI_COORD:
					print_and_log_trace(logger, "[NEW_ESI_CONNECTION]");
					header->type = HSK_ESI_COORD_OK;
					send_header(incoming_socket, header);

					pthread_t esi_thread_id;
					pthread_create(&esi_thread_id, NULL, esi_thread, incoming_socket);
					break;
			}
			free(header);
		}
		free(i_header);
	}
}

void * planner_thread() {
	print_and_log_info(logger, "[SAYING_HI_TO_PLANNER]");
	send_message_type(settings.planner_socket, HSK_PLANNER_COORD_OK);

	int max_sd, activity_socket;
	fd_set master_set;

	while(1) {
		FD_ZERO(&master_set);
		FD_SET(settings.planner_socket, &master_set);
		max_sd = settings.planner_socket;

		activity_socket = select(max_sd + 1, &master_set, NULL, NULL, NULL);

		//Recieve Header
		MessageHeader * i_header = malloc(sizeof(MessageHeader));
		if (recv(settings.planner_socket, i_header, sizeof(MessageHeader), MSG_PEEK) <= 0 ) {
			//Disconnected
			print_and_log_trace(logger, "[SOCKET_DISCONNECTED][PLANNER]");
		} else {
			//New message
			MessageHeader * header = malloc(sizeof(MessageHeader));

			switch(i_header->type){
				case GET_KEY_STATUS:
					recv(settings.planner_socket, i_header, sizeof(MessageHeader), 0);
					; //Statement vacio (C no permite declaraciones despues de etiquetas)
					char query_key[KEY_NAME_MAX];
					strcpy(query_key, i_header->comment);
					print_and_log_trace(logger, "[CONSOLE_ASKS_FOR_%s_STATUS]", query_key);

					StatusData * sd = malloc(sizeof(StatusData));
					sd->storage_exists = 0;
					sd->storage_isup = 0;

					ResourceRegistration * resource = search_resource(query_key);
					if(resource == NULL) {
						sd->real_key = 0;
						print_and_log_error(logger, "[RESOURCE_NOT_FOUND]");
					} else {
						sd->real_key = 1;
						InstanceRegistration * storage = resource->instance;

						print_and_log_error(logger, "[RESOURCE_FOUND]");

						if(storage != NULL){ //si se le asigno algun valor a la key
							strcpy(sd->actual_storage, storage->name);
							sd->storage_exists = 1;

							if(storage->isup){
								//Si su instancia no esta caida
								sd->storage_isup = 1;
								strcpy(sd->simulated_storage, sd->actual_storage);

								pthread_mutex_lock(&instance_feedback);
								i_header->type = COORD_ASKS_FOR_KEY_VALUE; //Reutilizo header recibido por plani (la key esta en header->comment)
								send_header(storage->socket, i_header);
								pthread_mutex_unlock(&instance_feedback);

								recieve_data(storage->socket, sd->key_value, sizeof(char) * KEY_VALUE_MAX);
							} else {
								strcpy(sd->key_value, "unable_to_get_value");
							}

							print_and_log_error(logger, "[WITH_VALUE_%s]", sd->key_value);
						} else {
							int instance_index = get_instance_index_by_alg(query_key, 1);
							InstanceRegistration * ir = list_get(instances, instance_index);
							strcpy(sd->simulated_storage, ir->name);
						}
					}

					send_data(settings.planner_socket, sd, sizeof(StatusData));
					free(sd);

					break;
			}
			free(header);
		}
		free(i_header);
	}
}

void * instance_thread(InstanceRegistration * ir) {
	MessageHeader * header = malloc(sizeof(MessageHeader));
	ir->isup = 1;
	instance_limit_calculation();

	pthread_mutex_init(&ir->available);
	pthread_mutex_init(&ir->gonnaread);
	pthread_mutex_lock(&ir->available);

	free(header);

	int max_sd, activity_socket;
	fd_set master_set;

	while(1) {
		FD_ZERO(&master_set);
		FD_SET(ir->socket, &master_set);
		max_sd = ir->socket;

		activity_socket = select(max_sd + 1, &master_set, NULL, NULL, NULL);

		if(ir->isup == 1) {
			MessageHeader * i_header = malloc(sizeof(MessageHeader));
			if (recv(ir->socket, i_header, sizeof(MessageHeader), MSG_PEEK) <= 0 ) {
				//Disconnected
				print_and_log_trace(logger, "[INSTANCE_SOCKET_DISCONNECTED][%s]", ir->name);
				ir->isup = 0;
				instance_limit_calculation();
			} else {
				//Allow op
				header = malloc(sizeof(MessageHeader));
				switch(i_header->type) {
					case IM_COMPACTING:
						recv(ir->socket, i_header, sizeof(MessageHeader), 0);
						print_and_log_info(logger, "[INSTANCE_TRIGGERED_COMPACT_%s]", ir->name);

						t_list * available_instances = list_filter(instances, (void *)is_up);

						for(int q=0 ; q<available_instances->elements_count ; q++) {
							InstanceRegistration * tir = list_get(available_instances, q);
							if(strcmp(tir->name, ir->name) != 0) {
								send_header(ir->socket, COMPACT_ORDER);
							}
						}

						pending_compacts = available_instances->elements_count;
						print_and_log_info(logger, "[LOCKING_INSTANCE_STEP_ALLOWANCE]");
						print_and_log_info(logger, "[WAITING_FOR_%d_COMPACTS]", pending_compacts);

						break;
					case DONE_COMPACTING:
						recv(ir->socket, i_header, sizeof(MessageHeader), 0);
						pending_compacts--;
						print_and_log_info(logger, "[INSTANCE_%s_DONE_COMPACTING]", ir->name);
						print_and_log_info(logger, "[WAITING_FOR_%d_COMPACTS]", pending_compacts);
						if(pending_compacts == 0) {
							print_and_log_info(logger, "[RESUMING_INSTANCE_ACTIVITY]");
							pthread_mutex_unlock(&ir->available);
						}
						break;
					case INSTRUCTION_OK_TO_COORD:
					case INSTRUCTION_FAILED_TO_COORD:
						print_and_log_info(logger, "DATA RECIEVED TOFREE");
						pthread_mutex_unlock(&ir->available);
						print_and_log_info(logger, "FREE");
						pthread_mutex_lock(&ir->gonnaread);
						pthread_mutex_lock(&ir->available);
						pthread_mutex_unlock(&ir->gonnaread);
						break;
					default:
						break;
				}
				free(header);
			}
			free(i_header);
		}
	}
}

void * esi_thread(int incoming_socket) {
	MessageHeader * header = malloc(sizeof(MessageHeader));
	int esi_alive = 1;

	while(esi_alive) {
		MessageHeader * i_header = malloc(sizeof(MessageHeader));
		if (recieve_header(incoming_socket, i_header) <= 0 ) {
			//Disconnected
			print_and_log_trace(logger, "[ESI_SOCKET_DISCONNECTED]");
			esi_alive = 0;
		} else {
			//New message

			header = malloc(sizeof(MessageHeader));
			switch(i_header->type) {
				case INSTRUCTION_ESI_COORD:
					print_and_log_trace(logger, "[INCOMING_INSTRUCTION_FROM_ESI]");
					InstructionDetail * instruction = malloc(sizeof(InstructionDetail));
					recieve_data(incoming_socket, instruction, sizeof(InstructionDetail));

					print_and_log_trace(logger, "[ESI_ID][ESI_%d]", instruction->esi_id);
					print_instruction(instruction);

					// Simulate delay
					print_and_log_trace(logger, "[DELAY]");
					sleep(settings.delay / 1000);

					// Get permissions from Planner
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

								print_and_log_info(logger, "[WAITING_FOR_INSTANCE_ALLOWANCE]");
								pthread_mutex_lock(&instance_feedback);
								print_and_log_info(logger, "[GRANTED]");
								send_header_and_data(instance->socket, header, instruction, sizeof(InstructionDetail));
								print_and_log_trace(logger, "[INSTRUCTION_SENT_TO_INSTANCE][%s]", instance->name);

								//Waiting for response
								pthread_mutex_lock(&instance->gonnaread);
								print_and_log_info(logger, "[WAITING_FOR_INSTANCE_ALLOWANCE]");
								pthread_mutex_lock(&instance->available);
								print_and_log_info(logger, "[GRANTED]");
								recieve_header(instance->socket, response_header);

								switch(response_header->type) {
									case INSTRUCTION_OK_TO_COORD:
										print_and_log_trace(logger, "[OPERATION_SUCCESSFUL][INFORMING_PLANNER]");

										recieve_data(instance->socket, &instance->free_entries, sizeof(int)); // Receiving free_entries from instance
										send_message_type(settings.planner_socket, INSTRUCTION_OK_TO_PLANNER);

										if(instruction->type != SET_OP) {
											print_and_log_trace(logger, "[NEW_RESOURCE_ALLOCATION][INFORMING_PLANNER]");
											ResourceAllocation * ra = malloc(sizeof(ResourceAllocation));
											ra->esi_id = instruction->esi_id;
											if(instruction->type == GET_OP) {
												log_info(operation_logger, "[ESI_%d][GET][%s]", instruction->esi_id, instruction->key);
												ra->type = BLOCKED;
											} else if(instruction->type == STORE_OP) {
												log_info(operation_logger, "[ESI_%d][STORE][%s]", instruction->esi_id, instruction->key);
												ra->type = RELEASED;
											}
											strcpy(ra->key, instruction->key);
											header->type = NEW_RESOURCE_ALLOCATION;
											send_header_and_data(settings.planner_socket, header, ra, sizeof(ResourceAllocation));
											free(ra);
										} else {
											log_info(operation_logger, "[ESI_%d][SET][%s][%s]", instruction->esi_id, instruction->key, instruction->opt_value);
										}

										break;
									case INSTRUCTION_FAILED_TO_COORD:
										print_and_log_trace(logger, "[OPERATION_FAILED][INFORMING_ESI]");
										send_message_type(incoming_socket, INSTRUCTION_FAILED_TO_ESI);
										break;
								}
								pthread_mutex_unlock(&instance->available);
								pthread_mutex_unlock(&instance->gonnaread);
								pthread_mutex_unlock(&instance_feedback);
							} else if (instruction->type == GET_OP) { //GET OP, allocated instance but no need to send instruction
								print_and_log_trace(logger, "[OPERATION_SUCCESSFUL][INFORMING_PLANNER]");
								send_message_type(settings.planner_socket, INSTRUCTION_OK_TO_PLANNER);
								print_and_log_trace(logger, "[NEW_RESOURCE_ALLOCATION][INFORMING_PLANNER]");

								log_info(operation_logger, "[ESI_%d][GET][%s]", instruction->esi_id, instruction->key);

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

t_list * instance_resources(InstanceRegistration * instance) {
	t_list * ret_resources = list_create();
	int a;
	for(a=0 ; a<resources->elements_count ; a++) {
		ResourceRegistration * re = list_get(resources, a);
		if(strcmp(re->instance->name, instance->name) == 0) {
			list_add(ret_resources, re);
		}
	}
	return ret_resources;
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
		if(!search_resource(instruction->key)){
			ResourceRegistration * re = malloc(sizeof(ResourceRegistration));
			re->instance = NULL;
			strcpy(re->key, instruction->key);
			list_add(resources, re);
		}
		return NULL;
	} else {
		ResourceRegistration * re = search_resource(instruction->key);
		if(re->instance == NULL) {
			re->instance = list_get(instances, get_instance_index_by_alg(instruction->key, 0));
		}
		return re->instance;
	}
	return NULL;
}

int find_first_lowercase (char *key){
    for (int i=0;i<(string_length(key)-1);i++){
        	int value = key[i];
        	if ((value >= 'a' ) && (value <= 'z')){
        		return value;
        	        }
    }
    return -1; //error?
}

bool is_up(InstanceRegistration *instance) {
	return (instance->isup == 1);
}

void instance_limit_calculation(){
	int letters = 26;
	t_list * instances_to_distribute = list_filter(instances, (void *)is_up);
	int inst_number = list_size(instances_to_distribute);
	if(inst_number == 0) return;
	int letters_per_instance = letters / inst_number;
	int first_letter = 'a';
	InstanceRegistration* inst;
	for(int i=0 ; i<instances->elements_count ; i++){
		inst= list_get(instances,i);
		if (inst->isup){
			inst->inf = first_letter;
			first_letter += letters_per_instance;
			inst->sup=first_letter;//first_letter ahora toma el valor de la ultima.
		}else{
			inst->inf=-1;
			inst->sup=-1;
		}
	}
	if(first_letter < 'z'){
		InstanceRegistration* last_inst = list_get(instances,list_size(instances)-1);
		last_inst->sup = 'z';
	}
}
bool max_free_entries_instance(InstanceRegistration *instance_1, InstanceRegistration * instance_2) {
	return (instance_1->free_entries > instance_2->free_entries);
}

int get_index_by_name(char name[INSTANCE_NAME_MAX]){
	for(int a=0 ; a<instances->elements_count ; a++) {
		InstanceRegistration * re = list_get(instances, a);
		if(strcmp(re->name, name) == 0) {
			return a;
		}
	}
	return -1;
}


int get_instance_index_by_alg(char *key, int simulation_mode) { //TODO algs
	int chosen_index = 0;
	int value = find_first_lowercase (key);
	int inst_number = list_size(instances);
	t_list * instances_to_distribute = list_filter(instances, (void *)is_up);
	InstanceRegistration* inst;

	if (instances_to_distribute->elements_count == 0) {
		print_and_log_info(logger, "[NO_AVALIABLE_INSTANCE]");
		return -1;
	}

	switch(settings.dist_alg) { // Switch between distribution algorithm
		case LSU:
			list_sort(instances_to_distribute, (void *)max_free_entries_instance);
			inst = list_get(instances_to_distribute, 0);
			chosen_index = get_index_by_name(inst->name);
			break;
		case EL:
			;
			int original_last = last_used_instance;

			last_used_instance++;
			if(last_used_instance == instances->elements_count) {
				last_used_instance = 0;
			}

			inst = list_get(instances, last_used_instance);
			while(!inst->isup) {
				last_used_instance++;
				if(last_used_instance == instances->elements_count) {
					last_used_instance = 0;
				}
				inst = list_get(instances, last_used_instance);
			}
			chosen_index = get_index_by_name(inst->name);

			if(simulation_mode == 1){
				last_used_instance = original_last;
			}
			break;
		case KE:
			for(int index=0 ; index<inst_number ; index++){
			inst = list_get(instances, index);
				if((value >= inst->inf) && (value <= inst->sup)) {
					chosen_index = get_index_by_name(inst->name);
				}
			}
			break;

		default:
			print_and_log_error(logger, "[DISTRIBUTION ALGORITHM NOT VALID][PLEASE SET A VALID DISTRIBUTION ALGORITHM]");
	}
	return chosen_index;
}
