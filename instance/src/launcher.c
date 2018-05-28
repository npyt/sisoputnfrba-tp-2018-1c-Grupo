#include "libgrupo/headers.h"

t_log * logger;
t_config * config;

t_list * blocks_table = NULL;
t_list * diccio_table = NULL;

int MAX_ENTRIES;
int ENTRY_SIZE_PARAM;
char * MOUNTING_POINT;
char * INSTANCE_NAME;

int circular_alg_last_entry;

int main() {
	logger = log_create("instance_logger.log", "INSTANCE", true, LOG_LEVEL_TRACE);
	config = config_create("instance_config.cfg");

	//Puerto para comunicarse con el coordinador
	int coordinator_socket = connect_with_server(config_get_string_value(config, "IP_COORD"),
			atoi(config_get_string_value(config, "PORT_COORD")));
	if(coordinator_socket < 0){
		log_error(logger, "ERROR AL CONECTAR CON EL COORDINADOR");
		return 1;
	} else {
		log_info(logger, "CONECTADO EN: %d", coordinator_socket);
	}

	MOUNTING_POINT = config_get_string_value(config, "MOUNTING_POINT");
	INSTANCE_NAME = config_get_string_value(config, "NAME");

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, coordinator_socket);
	pthread_exit(NULL);

	list_destroy(blocks_table);
	list_destroy(diccio_table);

	return 0;
}

void * listening_thread(int coordinator_socket) {
	log_info(logger, "[I_REQUEST_OK_TO_START_TO_COORD]");
	send_content_with_header(coordinator_socket, INSTANCE_COORD_HANDSHAKE, INSTANCE_NAME, sizeof(INSTANCE_NAME) * sizeof('a'));

	while(1) {
		MessageHeader * header = malloc(sizeof(MessageHeader));
		int rec = recv(coordinator_socket, header, sizeof(MessageHeader), 0);

		//Procesar el resto del mensaje dependiendo del tipo recibido
		switch((*header).type) {
			case INSTANCE_COORD_HANDSHAKE_OK:
				log_info(logger,"[COORDINATOR_OK_RECIEVED]");
				InstanceInitConfig * instance_config = malloc(sizeof(InstanceInitConfig));
				recv(coordinator_socket, instance_config, sizeof(InstanceInitConfig), 0);

				log_info(logger, "[CONFIG_RECIEVED][INSTANCES_Q=%d][ENTRY_SIZE=%d]", instance_config->entry_count, instance_config->entry_size);
				initialize_entry_table(instance_config->entry_count, instance_config->entry_size);
				free(instance_config);

				/*InstructionDetail * inst = malloc(sizeof(InstructionDetail));
				inst->operation = GET;
				strcpy(inst->key, "MESSI");
				process_instruction(inst);

				DiccionaryEntry * de = list_get(diccio_table, 0);
				EntryBlock * bl = list_get(blocks_table, de->entry_number);

				inst->operation = SET;
				strcpy(inst->key, "MESSI");
				char nv[] = "se lo meressi";
				inst->opt_value = malloc(sizeof(nv));
				strcpy(inst->opt_value, nv);
				process_instruction(inst);

				de = list_get(diccio_table, 0);
				bl = list_get(blocks_table, de->entry_number);

				dump_diccio_entry(0);*/
				break;
			case UNKNOWN_MSG_TYPE:
				log_error(logger, "[MY_MESSAGE_HASNT_BEEN_DECODED]");
				break;
			default:
				log_error(logger, "[UNKOWN_MESSAGE_RECIEVED]");
				send_only_header(coordinator_socket, UNKNOWN_MSG_TYPE);
				break;
		}
	}
}

void initialize_entry_table(int q_entries, int entry_size) {
	int a;
	blocks_table = list_create();
	diccio_table = list_create();
	MAX_ENTRIES = q_entries;
	ENTRY_SIZE_PARAM = entry_size;

	for(a=0 ; a<MAX_ENTRIES ; a++) {
		EntryBlock * element = malloc(sizeof(EntryBlock));
		element->entry_number = a;
		element->data = 0;
		element->data_size = 0;
		list_add(blocks_table, element);
	}

	circular_alg_last_entry = 0;
	log_info(logger, "[%d_ENTRIES_INIT]", blocks_table->elements_count);
}

void process_instruction(InstructionDetail * instruction) {
	switch(instruction->operation) {
		case GET_OP:
			{
				int a, found = 0;
				DiccionaryEntry * entry = -1;
				for(a=0 ; a<diccio_table->elements_count && found == 0 ; a++) {
					DiccionaryEntry * element = list_get(diccio_table, a);
					if ( strcmp(element->key, instruction->key) == 0 ) {
						found = 1;
						entry = element;
					}
				}
				if(found == 0) {
					entry = malloc(sizeof(DiccionaryEntry));
					int entry_index = get_index_to_replace();

					char def_val[] = "Default";
					strcpy(entry->key, instruction->key);
					entry->size = sizeof(def_val);
					entry->entry_number = entry_index;
					entry->blocked = 1;
					strcpy(entry->blockedBy, instruction->ESIName);

					list_add_in_index(diccio_table, 0, entry);
					copy_value_to_block(entry->entry_number, def_val, entry->size);

					circular_alg_last_entry++;
					if (circular_alg_last_entry == blocks_table->elements_count) {
						circular_alg_last_entry = 0;
					}

					log_info(logger, "[OPERATION_EXECUTED][GET][KEY=%s][CREATED_IN=%d]", instruction->key, entry_index);
				} else {
					entry->blocked = 1;
					strcpy(entry->blockedBy, instruction->ESIName);
					log_info(logger, "[OPERATION_EXECUTED][GET][KEY=%s]", instruction->key);
				}
				//TODO: Informar operacion ok
			}
			break;
		case SET_OP:
			{
				int a, found = 0;
				DiccionaryEntry * entry = -1;
				for(a=0 ; a<diccio_table->elements_count && found == 0 ; a++) {
					DiccionaryEntry * element = list_get(diccio_table, a);
					if ( strcmp(element->key, instruction->key) == 0 ) {
						found = 1;
						entry = element;
					}
				}
				if(found == 0) {
					log_error(logger, "[OPERATION_ABORTED][SET][SET_NOT_FOUND][KEY=%s]", instruction->key);
					//TODO: Informar operacion fallada
				} else {
					if(entry->blocked == 1 && strcmp(entry->blockedBy, instruction->ESIName) ) {
						entry->size = strlen(instruction->opt_value) * sizeof(char);
						copy_value_to_block(entry->entry_number, instruction->opt_value, entry->size);
					} else {
						log_error(logger, "[OPERATION_ABORTED][SET][KEY_NOT_BLOCKED_BY_ESI][KEY=%s]", instruction->key);
					}

					log_info(logger, "[OPERATION_EXECUTED][SET][KEY=%s]", instruction->key);
					//TODO: Informar operacion ok
				}
			}
			break;
		case STORE_OP:
			{
				int a, found = 0;
				DiccionaryEntry * entry = -1;
				for(a=0 ; a<diccio_table->elements_count && found == 0 ; a++) {
					DiccionaryEntry * element = list_get(diccio_table, a);
					if ( strcmp(element->key, instruction->key) == 0 ) {
						found = 1;
						entry = element;
						if(entry->blocked == 1 && strcmp(entry->blockedBy, instruction->ESIName) ) {
							entry->blocked = 0;
							dump_diccio_entry(a);
							log_info(logger, "[OPERATION_EXECUTED][STORE][KEY=%s]", instruction->key);
							//TODO: Informar operacion ok
						} else {
							log_error(logger, "[OPERATION_ABORTED][STORE][KEY_NOT_BLOCKED_BY_ESI][KEY=%s]", instruction->key);
							//TODO: Informar operacion fallada
						}
					}
				}
				if(found == 0) {
					log_error(logger, "[OPERATION_ABORTED][STORE_NOT_FOUND][KEY=%s]", instruction->key);
					//TODO: Informar operacion fallada
				}
			}
			break;
		default:
			log_error(logger, "[UNKNOWN_OPERATION_TYPE]");
			break;
	}
}

int get_index_to_replace() {
	return circular_alg_last_entry;
}

int copy_value_to_block(int fst_block, char * value, int size) {
	EntryBlock * block = list_get(blocks_table, fst_block);
	block->data_size = size;
	block->data = malloc(block->data_size);
	memcpy(block->data, value, block->data_size);
}

int dump_all_entries() {
	int a;
	for(a=0 ; a<diccio_table->elements_count ; a++) {
		dump_diccio_entry(a);
	}

	return 0;
}

int dump_diccio_entry(int index) {
	DiccionaryEntry * entry = list_get(diccio_table, index);
	EntryBlock * block = list_get(blocks_table, entry->entry_number);

	char * path = malloc((sizeof(MOUNTING_POINT) + RESOURCE_KEY_MAX_SIZE + 1) * sizeof(char));
	strcpy(path, MOUNTING_POINT);
	strcat(path, "/");
	strcat(path, entry->key);

	FILE * fl = fopen(path, "w");

	fwrite(block->data, 1, block->data_size, fl);

	close(fl);
	return 0;
}
