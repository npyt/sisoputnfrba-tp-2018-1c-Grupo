#include "libgrupo/headers.h"

t_log * logger;
t_config * config;
t_list * entries_table = NULL;
int MAX_ENTRIES;
int ENTRY_SIZE_PARAM;

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

	int server_socket = server_start(0);
		if (server_socket == -1) {
			log_error(logger, "ERROR AL INICIAR EL SERVIDOR");
			return 1;
		} else {
			log_info(logger, "SERVIDOR INICIADO");
		}

		if (listen(server_socket, 5) == -1) {
			log_error(logger, "ERROR AL ESCUCHAR EN PUERTO");
		} else {
			log_info(logger, "SERVIDOR ESCUCHANDO %d");
	}

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, server_socket);

	log_info(logger, "Envío saludo al coordinador");
	{
		int num = 1;
		send_content_with_header(coordinator_socket, INSTANCE_COORD_HANDSHAKE, &num, 0);
	}

	//Armo estructura para recibir la respuesta y la espero...
	MessageHeader * header = malloc(sizeof(MessageHeader));

	if(recv(coordinator_socket, header, sizeof(MessageHeader), 0) == -1) {
			log_error(logger, "Error al recibir el MessageHeader\n");
			return 1;
	}

	switch((*header).type) {
		case INSTANCE_COORD_HANDSHAKE_OK:
			log_info(logger,"El COORDINADOR me confirma que puedo iniciar la instancia, espero los datos de configuración");
			InstanceInitConfig * instance_config = malloc(sizeof(InstanceInitConfig));
			recv(coordinator_socket, instance_config, sizeof(InstanceInitConfig), 0);

			log_info(logger, "LA INTANCIA TENDRÁ %d ENTRADAS DE %d BYTES (COORDINATOR)", instance_config->entry_count, instance_config->entry_size);
			initialize_entry_table(instance_config->entry_count, instance_config->entry_size);
			free(instance_config);

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

	list_destroy(entries_table);

	return 0;
}

void * listening_thread(int server_socket) {

}

void initialize_entry_table(int q_entries, int entry_size) {
	t_list * entries_table = list_create();
	MAX_ENTRIES = q_entries;
	ENTRY_SIZE_PARAM = entry_size;
}

void process_instruction(InstructionDetail * instruction) {
	switch(instruction->operation) {
		case GET:
			log_info(logger, "OPERACIÓN DE GET");
			break;
		case SET:
			log_info(logger, "OPERACIÓN DE SET");
			break;
		case STORE:
			log_info(logger, "OPERACIÓN DE STORE");
			break;
		default:
			log_error(logger, "ERROR en el tipo de operación en la instrucción a procesar.");
			break;
	}
}
