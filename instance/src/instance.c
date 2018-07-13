#include <chucknorris/allheaders.h>

t_config * config;
t_log * logger;

t_list * storage_cells;
t_list * resources;

InstanceConfig settings;
InstanceData entry_settings;

int last_used_cell;
int local_operation_counter;

void * listening_thread(int server_socket);
void * dump_thread();
char * get_file_path(char key[KEY_NAME_MAX]);

ResourceStorage * get_key(char key[KEY_NAME_MAX], int force_creation);

pthread_mutex_t allow_listening;

int main(int argc, char **argv) {
	if(argv[1] == NULL) {
		//exit_with_message("No especificó el archivo de configuración.", EXIT_FAILURE);
		argv[1] = malloc(sizeof(char) * 1024);
		strcpy(argv[1], "config3.cfg");
	}

	config = config_create(argv[1]);
	logger = log_create("log.log", "INSTANCE", false, LOG_LEVEL_TRACE);

	strcpy(settings.coord_ip, config_get_string_value(config, "COORD_IP"));
	settings.coord_port = config_get_int_value(config, "COORD_PORT");
	strcpy(settings.mounting_point, config_get_string_value(config, "MOUNTING_POINT"));
	strcpy(settings.name, config_get_string_value(config, "NAME"));
	settings.dump = config_get_int_value(config, "DUMP");
	char * buffer = malloc(128 * sizeof(char));
	strcpy(buffer, config_get_string_value(config, "REPLACEMENT_ALG"));
	if(strcmp(buffer, "CIRC") == 0) {
		settings.replacement_alg = CIRC;
	} else if(strcmp(buffer, "LRU") == 0) {
		settings.replacement_alg = LRU;
	} else if(strcmp(buffer, "BSU") == 0) {
		settings.replacement_alg = BSU;
	}
	free(buffer);
	config_destroy(config);

	pthread_mutex_init(&allow_listening, NULL);

	storage_cells = NULL;
	resources = NULL;

	local_operation_counter = -1;

	int coordinator_socket = connect_with_server(settings.coord_ip, settings.coord_port);

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, coordinator_socket);
	//pthread_t dump_thread_id;
	//pthread_create(&dump_thread_id, NULL, dump_thread, NULL);

	pthread_exit(NULL);

	return EXIT_SUCCESS;
}

void * dump_thread() {
	while(1) {
		sleep(settings.dump);
		pthread_mutex_lock(&allow_listening);
		print_and_log_trace(logger, "[DUMP_STARTED]");
		if(resources != NULL) {
			for(int a=0 ; a<resources->elements_count ; a++) {
				dump_storage(list_get(resources, a));
			}
		}
		sleep(5);
		print_and_log_trace(logger, "[DUMP_FINISHED]");
		pthread_mutex_unlock(&allow_listening);
	}
}

void map_cells() {
	if(storage_cells != NULL){
		for(int o=0 ; o<storage_cells->elements_count ; o++) {
			StorageCell * sc = list_get(storage_cells, o);
			if(sc->content_size > 0) {
				print_and_log_info(logger, "\t%d\t%s", o, sc->content);
			} else {
				print_and_log_info(logger, "\t%d", o);
			}
		}
	}
}

void * listening_thread(int coordinator_socket) {
	settings.coordinator_socket = coordinator_socket;
	int clients[MAX_SERVER_CLIENTS];
	int a, max_sd, activity_socket, incoming_socket;
	fd_set master_set;
	for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
		clients[a] = 0;
	}
	clients[0] = coordinator_socket;

	send_message_type(coordinator_socket, HSK_INST_COORD);
	send_data(coordinator_socket, settings.name, INSTANCE_NAME_MAX);
	print_and_log_trace(logger, "[SAYING_HI_TO_COORDINATOR]");

	while(1) {
		FD_ZERO(&master_set);
		for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
			if(clients[a] > 0) {
				FD_SET(clients[a], &master_set);
				if(max_sd < clients[a]) {
					max_sd = clients[a];
				}
			}
		}

		activity_socket = select(max_sd + 1, &master_set, NULL, NULL, NULL);
		pthread_mutex_lock(&allow_listening);

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

					switch(i_header->type) {
						case HSK_INST_COORD_OK :
							print_and_log_trace(logger, "[COORDINATOR_SAYS_HI]");
							print_and_log_trace(logger, "[REPORTING AS %s]", settings.name);
							recieve_data(incoming_socket, &entry_settings, sizeof(InstanceData));

							print_and_log_trace(logger, "[COORDINATOR_GIVES_CONFIG][%d_ENTRIES][%d_SIZE]", entry_settings.entry_count, entry_settings.entry_size);

							prepare_storage();
							break;
						case HSK_INST_COORD_RELOAD: //If i was down, must reload previous keys from the mounting point
							print_and_log_trace(logger, "[COORDINATOR_SAYS_HI][I_MUST_RELOAD_PREVIOUS_KEYS]");

							recieve_data(incoming_socket, &entry_settings, sizeof(InstanceData));

							print_and_log_trace(logger, "[COORDINATOR_GIVES_CONFIG][%d_ENTRIES][%d_SIZE]", entry_settings.entry_count, entry_settings.entry_size);
							prepare_storage();

							int key_count;
							recieve_data(incoming_socket, &key_count, sizeof(int));

							char * buffer;
							char * value;
							InstructionDetail * t_instruction;

							for(int a=0 ; a<key_count ; a++) {
								buffer = malloc(sizeof(char) * KEY_NAME_MAX);
								recieve_data(incoming_socket, buffer, sizeof(char) * KEY_NAME_MAX);
								print_and_log_trace(logger, "[LOAD_KEY_%s]", buffer);

								value = malloc(sizeof(char) * KEY_VALUE_MAX);
								FILE * f = fopen(get_file_path(buffer), "r");

								if(!f) {
									strcpy(value, "no_value");
								} else {
									value[fread(value, sizeof(char), KEY_VALUE_MAX, f)] = '\0';
									fclose(f);
								}

								print_and_log_trace(logger, "  [WITH_VALUE_%s]", value);

								t_instruction = malloc(sizeof(InstructionDetail));
								t_instruction->esi_id = -1;
								strcpy(t_instruction->key, buffer);
								strcpy(t_instruction->opt_value, value);

								t_instruction->type = SET_OP;
								process_instruction(t_instruction);

								free(t_instruction);
							}

							break;
						case INSTRUCTION_COORD_INST:
							print_and_log_trace(logger, "[INCOMING_INSTRUCTION_FROM_COORDINATOR]");
							print_and_log_trace(logger, "[PROCESSING]");

							local_operation_counter++;

							InstructionDetail * instruction = malloc(sizeof(InstructionDetail));
							recieve_data(incoming_socket, instruction, sizeof(InstructionDetail));

							print_instruction(instruction);
							if(process_instruction(instruction) == 1) {
								print_and_log_trace(logger, "[INSTRUCTION_SUCCESSFUL][INFORMING_COORDINATOR]");
								send_message_type(incoming_socket, INSTRUCTION_OK_TO_COORD);
								print_and_log_trace(logger, "[SAMPAOLI]");
								send_data(incoming_socket, &settings.free_entries, sizeof(int)); //sending free_entries to coordinator
							} else {
								print_and_log_trace(logger, "[INSTRUCTION_FAILED][INFORMING_COORDINATOR]");
								send_message_type(incoming_socket, INSTRUCTION_FAILED_TO_COORD);
							}
							break;
						case COORD_ASKS_FOR_KEY_VALUE:
							; //Statement vacio (C no permite declaraciones despues de etiquetas)
							ResourceStorage * rs = malloc(sizeof(ResourceStorage));

							rs = get_key(i_header->comment, 0);
							print_and_log_trace(logger, "[COORDINATOR_ASKING_FOR_%s_VALUE]", i_header->comment);
							int a = 0;

							char * stored_value = malloc(KEY_VALUE_MAX * sizeof(char));
							for(int i=0 ; i<KEY_VALUE_MAX ; i++) {
								stored_value[i] = '\0';
							}

							StorageCell * cell;
							for(int q=0 ; q<rs->cell_count ; q++){
								cell = list_get(storage_cells, rs->cell_id + q);
								for(int i=0 ; i<strlen(cell->content) ; i++) {
									stored_value[a++] = cell->content[i];
								}
							}

							print_and_log_trace(logger, "[VALUE_%s]", stored_value);

							send_data(incoming_socket, stored_value, sizeof(char) * KEY_VALUE_MAX);
							free(stored_value);
							break;
						case COMPACT_ORDER:
							print_and_log_trace(logger, "[COORDINATOR_ORDERS_ME_TO_COMPACT]");
							compact(1);
							break;
					}

					free(header);
				}

				free(i_header);
			}
		}
		pthread_mutex_unlock(&allow_listening);
	}
}

void prepare_storage() {
	int a;
	storage_cells = list_create();
	resources = list_create();
	for(a=0 ; a<entry_settings.entry_count ; a++) {
		StorageCell * cell = malloc(sizeof(StorageCell));
		cell->atomic_value = 0;
		cell->content = NULL;
		cell->content_size = 0;
		cell->id = a;
		cell->last_reference = -1;

		list_add(storage_cells, cell);
	}

	//Start administrative structures for replacement algorithms
	settings.free_entries = entry_settings.entry_count;
	settings.atomic_entries = 0;
	last_used_cell = -1;
}

int process_instruction(InstructionDetail * instruction) {
	ResourceStorage * rs = malloc(sizeof(ResourceStorage));
	switch(instruction->type) {
		//Instance doesnt process GET op, Issue #1082
		case SET_OP:
			if((rs = get_key(instruction->key, 1)) != NULL) {
				return set_storage(rs, instruction->opt_value);
			} else {
				return 0;
			}
			break;
		case STORE_OP:
			if((rs = get_key(instruction->key, 0)) != NULL) {
				return store_storage(rs);
			} else {
				return 0;
			}
			break;
	}
	return 0;
}

ResourceStorage * search_resource_by_cell_id(int id) {
	for(int a=0 ; a<resources->elements_count ; a++) {
		ResourceStorage * rs = list_get(resources, a);
		if(rs->cell_id == id) return rs;
	}
	return NULL;
}

int compact(int from_coordinator) {
	if(!from_coordinator) {
		send_message_type(settings.coordinator_socket, IM_COMPACTING);
	} else {
		pthread_mutex_lock(&allow_listening);
	}
	print_and_log_trace(logger, "[COMPACT_START]");
	int allocated_cells = entry_settings.entry_count - settings.free_entries;
	int destination_cell = 0, parse_index = 0, new_start;

	StorageCell * sc;

	while(parse_index < entry_settings.entry_count) {
		sc = list_get(storage_cells, parse_index);
		while(parse_index < entry_settings.entry_count && sc->content_size == 0) {
			parse_index++;
			if(parse_index < entry_settings.entry_count){
				sc = list_get(storage_cells, parse_index);
			}
		}
		if(parse_index == entry_settings.entry_count) {
		} else {
			ResourceStorage * rs = search_resource_by_cell_id(sc->id);
			if (rs != NULL) {
				if(rs->cell_id != destination_cell) {
					new_start = destination_cell;
					for(int q=0 ; q<rs->cell_count ; q++) {
						StorageCell * or = list_get(storage_cells, rs->cell_id + q);
						StorageCell * de = list_get(storage_cells, destination_cell);

						de->atomic_value = or->atomic_value;
						de->content_size = or->content_size;
						de->content = or->content;
						de->last_reference = or->last_reference;

						or->atomic_value = 0;
						or->content_size = 0;
						or->content = NULL;
						or->last_reference = -1;

						destination_cell++;
					}
					rs->cell_id = new_start;
					parse_index = destination_cell;
				} else {
					destination_cell += rs->cell_count;
					parse_index += rs->cell_count;
				}
			}
		}
	}
	send_message_type(settings.coordinator_socket, DONE_COMPACTING);
	print_and_log_trace(logger, "[COMPACT_END]");

	if(from_coordinator) {
		pthread_mutex_lock(&allow_listening);
	}
	return destination_cell;
}

ResourceStorage * get_key(char key[KEY_NAME_MAX], int force_creation) {
	int a, found = 0;
	for(a=0 ; a<resources->elements_count ; a++) {
		ResourceStorage * rs = list_get(resources, a);
		if(strcmp(rs->key, key) == 0) {
			found = 1;
			return rs;
		}
	}
	if(found == 0 && force_creation) {
		ResourceStorage * rs = malloc(sizeof(ResourceStorage));
		rs->cell_id = -1;
		rs->size = 0;
		rs->cell_count = 0;
		strcpy(rs->key, key);

		print_and_log_trace(logger, "[ALLOCATED_NEW_KEY][%s]", rs->key);
		list_add(resources, rs);

		return rs;
	}
	return NULL;
}

int are_there_n_free_cells_from(int start_index, int n_cells) {
	int a = start_index;
	if(start_index + n_cells > (entry_settings.entry_count)) {
		return 0;
	}
	for( ; a<start_index+n_cells ; a++) {
		StorageCell * cell = list_get(storage_cells, a);
		if(cell->content_size != 0) {
			return 0;
		}
	}
	return 1;
}

int are_there_n_atomic_or_free_cells_from(int start_index, int n_cells) {
	int a = start_index;
	if(start_index + n_cells > (entry_settings.entry_count)) {
		return 0;
	}
	for( ; a<start_index+n_cells ; a++) {
		StorageCell * cell = list_get(storage_cells, a);
		if(cell->atomic_value == 0 && cell->content_size != 0) {
			return 0;
		}
	}
	return 1;
}

void free_cell(StorageCell * sc) {
	settings.free_entries++;
	settings.atomic_entries--;

	for(int a=0 ; a<resources->elements_count ; a++) {
		ResourceStorage * rs = list_get(resources, a);
		if(rs->cell_id == sc->id) {
			list_remove(resources, a);
		}
	}

	sc->atomic_value = 0;
	free(sc->content);
	sc->content = NULL;
	sc->content_size = 0;
	sc->last_reference = -1;
}

int set_storage(ResourceStorage * rs, char value[KEY_VALUE_MAX]) {
	int bytes_needed = strlen(value) * sizeof(char);
	float cells_needed_f = (float)bytes_needed / (float)entry_settings.entry_size;
	int cells_needed = bytes_needed / entry_settings.entry_size;
	if(cells_needed_f > cells_needed) {
		cells_needed++;
	}

	if(rs->cell_id == -1) { //If its first SET op on key
		if(cells_needed > entry_settings.entry_count ||
				cells_needed > settings.free_entries + settings.atomic_entries) {
			return -1;
		}

		while(cells_needed > settings.free_entries) { //Must replace
			int freed = -1;
			switch(settings.replacement_alg) {
				case CIRC:
					{
						StorageCell * sc;
						for(int tries=0 ; tries<entry_settings.entry_count && freed == -1; tries++) {
							last_used_cell++;
							if(last_used_cell >= storage_cells->elements_count) { last_used_cell = 0; }

							sc = list_get(storage_cells, last_used_cell);
							if(sc->atomic_value) {
								free_cell(sc);
								freed = 1;
							}
						}
					}
					break;
				case LRU:
					{
						int min_lsu, index_lsu;
						StorageCell * sc;
						min_lsu = local_operation_counter;
						index_lsu = -1;
						for(int a=0 ; a<storage_cells->elements_count ; a++) {
							sc = list_get(storage_cells, a);
							if(sc->last_reference < min_lsu) {
								if(sc->atomic_value) {
									min_lsu = sc->last_reference;
									index_lsu = a;
								}
							}
						}
						sc = list_get(storage_cells, index_lsu);
						free_cell(sc);
					}
					break;
				case BSU:
				{
					int bsu_index = -1;
					int bsu_size =  -1;
					StorageCell* sc;
					for(int a=0; a<storage_cells->elements_count; a++){
					sc = list_get(storage_cells, a);

					if(sc->atomic_value){
						if(sc->content_size > bsu_size){
							bsu_size = sc->content_size;
							bsu_index = a;
						}
					}
				}
					sc = list_get(storage_cells, bsu_index);
					free_cell(sc);
					break;
				}
			}
		}
		{ //Once I got the free ones, loop to seek adjoining cells
			for(int a=0 ; a<entry_settings.entry_count && rs->cell_id == -1; a++) {
				if(are_there_n_free_cells_from(a, cells_needed)) {
					rs->cell_id = a;
					last_used_cell = a;
				}
			}

			if(rs->cell_id == -1) {
				last_used_cell = compact(0);
				rs->cell_id = last_used_cell;
			}

			last_used_cell = rs->cell_id + (cells_needed-1);
			if(last_used_cell > entry_settings.entry_count) {
				last_used_cell = 0;
			}
		}
	} else {
		if (cells_needed > rs->cell_count) return -1;
	}

	rs->size = strlen(value) * sizeof(char);
	for(int q=0 ; q<rs->cell_count ; q++) { //Clear prior cells
		StorageCell * cell = list_get(storage_cells, rs->cell_id + q);
		cell->atomic_value = 0;
		free(cell->content);
		cell->content = NULL;
		cell->content_size = 0;
		cell->last_reference = -1;
		settings.free_entries++;
	}
	if(rs->cell_count == 1) {
		settings.atomic_entries--;
	}
	rs->cell_count = cells_needed;

	print_and_log_trace(logger, "[START_ALLOCATION][KEY_%s][INIT_ENTRY_%d]", rs->key, rs->cell_id);

	for(int q=0 ; q<cells_needed ;q++) {
		StorageCell * cell = list_get(storage_cells, rs->cell_id + q);
		if(cells_needed == 1) {
			cell->atomic_value = 1;
		} else {
			cell->atomic_value = 0;
		}
		settings.free_entries--;
		cell->content_size = entry_settings.entry_size;
		cell->content = malloc(cell->content_size);
		cell->last_reference = local_operation_counter;
		memcpy(cell->content, value+q*entry_settings.entry_size, entry_settings.entry_size);
		cell->content_size = strlen(cell->content);
		print_and_log_trace(logger, "[ENTRY_%d][ATOMIC_%d][VALUE_%s]", cell->id, cell->atomic_value, cell->content);
	}
	if(cells_needed == 1) {
		settings.atomic_entries++;
	}


	//print_and_log_trace(logger, "MAPPING MEMORY ; LU %d", last_used_cell);
	//map_cells();
	return 1;
}

int store_storage(ResourceStorage * rs) {
	print_and_log_trace(logger, "[STORED_KEY][%s]", rs->key);
	return dump_storage(rs);
}

char * get_file_path(char key[KEY_NAME_MAX]) {
	char * file_name = malloc(sizeof(char) * ( strlen(settings.mounting_point) + strlen(settings.name) + KEY_NAME_MAX + 3 ));
	strcpy(file_name, settings.mounting_point);
	strcat(file_name, key);
	return file_name;
}

int dump_storage(ResourceStorage * rs) {
	char * file_name = get_file_path(rs->key);

	FILE * fd = fopen(file_name, "w");
	if(fd) {
		if(rs->size != 0) {
			for(int q=0 ; q<rs->cell_count ; q++) {
				StorageCell * cell = list_get(storage_cells, rs->cell_id + q);
				cell->last_reference = local_operation_counter;
				fwrite(cell->content, cell->content_size, 1, fd);
			}
		} else {
		}
		fclose(fd);
	} else {
		print_and_log_error(logger, "[COULD_NOT_OPEN_FILE]");
	}

	print_and_log_trace(logger, "[DUMPED_KEY][%s]", rs->key);
	fflush(stdout);

	return 1;
}
