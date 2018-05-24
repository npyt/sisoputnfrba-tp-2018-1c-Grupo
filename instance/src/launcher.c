#include "libgrupo/headers.h"

t_log * logger;
t_config * config;

t_list * blocks_table = NULL;
t_list * diccio_table = NULL;

int MAX_ENTRIES;
int ENTRY_SIZE_PARAM;
char * MOUNTING_POINT;

int circular_alg_last_entry;

int main() {
	logger = log_create("instance_logger.log", "INSTANCE", true, LOG_LEVEL_TRACE);
	config = config_create("instance_config.cfg");

	int coordinator_socket = connect_with_server(config_get_string_value(config, "IP_COORD"),
			atoi(config_get_string_value(config, "PORT_COORD")));

	if(coordinator_socket < 0){
		log_error(logger, " ERROR AL CONECTAR CON EL COORDINADOR");
		return 1;
	} else {
		log_info(logger, " CONECTADO EN: %d", coordinator_socket);
	}

	log_info(logger, "Envío saludo al coordinador");
	{
		int num = 1;
		send_content_with_header(coordinator_socket, INSTANCE_COORD_HANDSHAKE, &num, 0);
	}

	MOUNTING_POINT = config_get_string_value(config, "MOUNTING_POINT");

	MessageHeader * header = malloc(sizeof(MessageHeader));
	recv(coordinator_socket, header, sizeof(MessageHeader), 0);
	switch((*header).type) {
		case INSTANCE_COORD_HANDSHAKE_OK:
			log_info(logger,"El COORDINADOR me confirma que puedo iniciar la instancia, espero los datos de configuración");
			InstanceInitConfig * instance_config = malloc(sizeof(InstanceInitConfig));
			recv(coordinator_socket, instance_config, sizeof(InstanceInitConfig), 0);

			log_info(logger, "LA INTANCIA TENDRÁ %d ENTRADAS DE %d BYTES (COORDINATOR)", instance_config->entry_count, instance_config->entry_size);
			initialize_entry_table(instance_config->entry_count, instance_config->entry_size);
			free(instance_config);

			InstructionDetail * inst = malloc(sizeof(InstructionDetail));
			inst->operation = GET;
			strcpy(inst->key, "MESSI");
			process_instruction(inst);

			DiccionaryEntry * de = list_get(diccio_table, 0);
			EntryBlock * bl = list_get(blocks_table, de->entry_number);
			log_info(logger, "%s = %s", de->key, bl->data);

			inst->operation = SET;
			strcpy(inst->key, "MESSI");
			char nv[] = "se lo meressi";
			inst->opt_value = malloc(sizeof(nv));
			strcpy(inst->opt_value, nv);
			process_instruction(inst);

			de = list_get(diccio_table, 0);
			bl = list_get(blocks_table, de->entry_number);
			log_info(logger, "%s = %s", de->key, bl->data);

			dump_diccio_entry(0);

			break;
		case UNKNOWN_MSG_TYPE:
			log_error(logger, "El COORDINADOR no reconoció el último mensaje enviado");
			break;
		default:
			log_error(logger, "No reconozco el tipo de mensaje");
			{
				int num = 1;
				send_content_with_header(coordinator_socket, UNKNOWN_MSG_TYPE, &num, 0);
			}
			break;
	}

	list_destroy(blocks_table);
	list_destroy(diccio_table);

	return 0;
}

void * listening_thread(int server_socket) {

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
	log_info(logger, "ENTRADAS INICIALIZADAS %d", blocks_table->elements_count);
}

void process_instruction(InstructionDetail * instruction) {
	switch(instruction->operation) {
		case GET:
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

					list_add_in_index(diccio_table, 0, entry);
					copy_value_to_block(entry->entry_number, def_val, entry->size);

					circular_alg_last_entry++;
					if (circular_alg_last_entry == blocks_table->elements_count) {
						circular_alg_last_entry = 0;
					}

					log_info(logger, "OPERACIÓN GET SOBRE %s (creado en entry %d)", instruction->key, entry_index);
				} else {
					log_info(logger, "OPERACIÓN GET SOBRE %s", instruction->key);
				}
				//TODO: Informar operacion ok
			}
			break;
		case SET:
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
					log_error(logger, "Se intentó hacer SET sobre una entrada inexistente (KEY = %s)", instruction->key);
					//TODO: Informar operacion fallada
				} else {
					entry->size = strlen(instruction->opt_value) * sizeof(char);
					copy_value_to_block(entry->entry_number, instruction->opt_value, entry->size);

					log_info(logger, "OPERACIÓN SET SOBRE %s", instruction->key);
					//TODO: Informar operacion ok
				}
			}
			break;
		case STORE:
			{
				int a, found = 0;
				DiccionaryEntry * entry = -1;
				for(a=0 ; a<diccio_table->elements_count && found == 0 ; a++) {
					DiccionaryEntry * element = list_get(diccio_table, a);
					if ( strcmp(element->key, instruction->key) == 0 ) {
						found = 1;
						entry = element;
						dump_diccio_entry(a);
						log_info(logger, "OPERACIÓN STORE SOBRE %s", instruction->key);
						//TODO: Informar operacion ok
					}
				}
				if(found == 0) {
					log_error(logger, "Se intentó hacer STORE sobre una entrada inexistente (KEY = %s)", instruction->key);
					//TODO: Informar operacion fallada
				}
			}
			break;
		default:
			log_error(logger, "ERROR en el tipo de operación en la instrucción a procesar.");
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
	log_info(logger, "KEY DUMPEADA %s", entry->key);
}
