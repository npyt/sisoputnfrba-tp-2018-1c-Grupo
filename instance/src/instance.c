#include <chucknorris/allheaders.h>

t_config * config;
t_log * logger;

t_list * storage_cells;
t_list * resources;

InstanceConfig settings;
InstanceData entry_settings;

int last_used_cell;

void * listening_thread(int server_socket);

ResourceStorage * get_key(char key[KEY_NAME_MAX]);

int main(int argc, char **argv) {
	if(argv[1] == NULL) {
		//exit_with_message("No especificó el archivo de configuración.", EXIT_FAILURE);
		argv[1] = malloc(sizeof(char) * 1024);
		strcpy(argv[1], "config.cfg");
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

	storage_cells = NULL;
	resources = NULL;

	int coordinator_socket = connect_with_server(settings.coord_ip, settings.coord_port);

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, coordinator_socket);

	pthread_exit(NULL);

	return EXIT_SUCCESS;
}

void * listening_thread(int coordinator_socket) {
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

							load_previous_keys();
							break;
						case INSTRUCTION_COORD_INST:
							print_and_log_trace(logger, "[INCOMING_INSTRUCTION_FROM_COORDINATOR]");
							print_and_log_trace(logger, "[PROCESSING]");

							InstructionDetail * instruction = malloc(sizeof(InstructionDetail));
							recieve_data(incoming_socket, instruction, sizeof(InstructionDetail));

							print_instruction(instruction);
							if(process_instruction(instruction) == 1) {
								print_and_log_trace(logger, "[INSTRUCTION_SUCCESSFUL][INFORMING_COORDINATOR]");
								send_message_type(incoming_socket, INSTRUCTION_OK_TO_COORD);
								send_data(incoming_socket, &settings, sizeof(InstanceConfig)); //sending free_entries to coordinator
							} else {
								print_and_log_trace(logger, "[INSTRUCTION_FAILED][INFORMING_COORDINATOR]");
								send_message_type(incoming_socket, INSTRUCTION_FAILED_TO_COORD);
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

		list_add(storage_cells, cell);
	}

	//Start administrative structures for replacement algorithms
	settings.free_entries = entry_settings.entry_count;
	last_used_cell = -1;
}

void load_previous_keys() {
	//TODO Load keys and values from the mounting point
}

int process_instruction(InstructionDetail * instruction) {
	ResourceStorage * rs = malloc(sizeof(ResourceStorage));
	switch(instruction->type) {
		//Instance doesnt process GET op, Issue #1082
		case SET_OP:
			if((rs = get_key(instruction->key)) != NULL) {
				return set_storage(rs, instruction->opt_value);
			} else {
				return 0;
			}
			break;
		case STORE_OP:
			if((rs = get_key(instruction->key)) != NULL) {
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

int compact() {
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

						or->atomic_value = 0;
						or->content_size = 0;
						or->content = NULL;

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
	return destination_cell;
}

ResourceStorage * get_key(char key[KEY_NAME_MAX]) {
	int a, found = 0;
	for(a=0 ; a<resources->elements_count ; a++) {
		ResourceStorage * rs = list_get(resources, a);
		if(strcmp(rs->key, key) == 0) {
			found = 1;
			return rs;
		}
	}
	if(found == 0) {
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

int set_storage(ResourceStorage * rs, char value[KEY_VALUE_MAX]) {
	int bytes_needed = strlen(value) * sizeof(char);
	float cells_nedded_f = (float)bytes_needed / (float)entry_settings.entry_size;
	int cells_nedded = bytes_needed / entry_settings.entry_size;
	if(cells_nedded_f > cells_nedded) {
		cells_nedded++;
	}

	if(rs->cell_id == -1) { //If its first SET op on key
		if(cells_nedded > entry_settings.entry_count) {
			return -1;
		}

		if(cells_nedded > settings.free_entries) { //Must replace
			int tries, final_index = -1;
			switch(settings.replacement_alg) {
				case CIRC:
					for(tries=0 ; tries<entry_settings.entry_count && final_index == -1; tries++) {
						last_used_cell++;
						if(last_used_cell >= storage_cells->elements_count) { last_used_cell = 0; }

						print_and_log_debug(logger, "    ARE THERE %d FREE FROM %d", cells_nedded, last_used_cell);
						if(are_there_n_atomic_or_free_cells_from(last_used_cell, cells_nedded) == 1) {
							final_index = last_used_cell;
						}
					}
					break;
				case LRU:
					break;
				case BSU:
					break;
			}
			rs->cell_id = final_index;
		} else {
			last_used_cell++;
			if(last_used_cell == storage_cells->elements_count) { last_used_cell = 0; }

			if(are_there_n_free_cells_from(last_used_cell, cells_nedded) == 1) { //See if there is free space from last used
				rs->cell_id = last_used_cell;
			} else { //If not, must compact and look again from last position
				last_used_cell = compact();
				rs->cell_id = last_used_cell;
			}
		}

	} else {
		if (cells_nedded > rs->cell_count) return -1;
	}

	rs->size = strlen(value) * sizeof(char);
	for(int q=0 ; q<rs->cell_count ; q++) { //Clear prior cells
		StorageCell * cell = list_get(storage_cells, rs->cell_id + q);
		cell->atomic_value = 0;
		free(cell->content);
		cell->content = NULL;
		cell->content_size = 0;
		settings.free_entries++;
	}
	rs->cell_count = cells_nedded;

	print_and_log_trace(logger, "[START_ALLOCATION][KEY_%s][INIT_ENTRY_%d]", rs->key, rs->cell_id);

	for(int q=0 ; q<cells_nedded ;q++) {
		StorageCell * cell = list_get(storage_cells, rs->cell_id + q);
		if(cells_nedded == 1) {
			cell->atomic_value = 1;
		} else {
			cell->atomic_value = 0;
		}
		settings.free_entries--;
		cell->content_size = entry_settings.entry_size;
		cell->content = malloc(cell->content_size);
		memcpy(cell->content, value+q*entry_settings.entry_size, entry_settings.entry_size);

		if(rs->cell_id + q > last_used_cell) {
			last_used_cell = rs->cell_id + q;
		}
		print_and_log_trace(logger, "[ENTRY_%d][ATOMIC_%d][VALUE_%s]", cell->id, cell->atomic_value, cell->content);
	}
	return 1;
}

int store_storage(ResourceStorage * rs) {
	print_and_log_trace(logger, "[STORED_KEY][%s]", rs->key);
	return dump_storage(rs);
}

int dump_storage(ResourceStorage * rs) {
	char * file_name = malloc(sizeof(char) * ( strlen(settings.mounting_point) + strlen(settings.name) + KEY_NAME_MAX + 3 ));
	strcpy(file_name, settings.mounting_point);
	strcat(file_name, rs->key);


	FILE * fd = fopen(file_name, "wb+");
	if(rs->size != 0) {
		for(int q=0 ; q<rs->cell_count ; q++) {
			StorageCell * cell = list_get(storage_cells, rs->cell_id + q);
			fwrite(cell->content, cell->content_size, 1, fd);
		}
	} else {
		//No content
	}
	fclose(fd);

	print_and_log_trace(logger, "[DUMPED_KEY][%s]", rs->key);
	fflush(stdout);

	return 1;
}
