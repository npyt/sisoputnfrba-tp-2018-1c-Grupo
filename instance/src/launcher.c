#include "libgrupo/headers.h"

t_log * logger;
t_config * config;

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
	return 0;
}
