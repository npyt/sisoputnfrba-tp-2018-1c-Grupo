#include "headers.h"

t_log * logger;
t_config * config;

typedef struct{
	char buffer[100];
}InstanceConnection;


int main() {
	logger = log_create("instance_logger.log", "INSTANCE", true, LOG_LEVEL_TRACE);
	config = config_create("instance_config.cfg");


	int coordinator_socket = connect_with_server( "127.0.0.1", 8000);

		if(coordinator_socket < 0){
					log_error(logger, " ERROR AL CONECTAR CON EL COORDINADOR");
						return 1;
				}else{
					log_info(logger, " CONECTADO EN: %d", coordinator_socket);
					}

		//Armo una estructura para enviar al coordinador
		InstanceConnection data;
		strcpy(data.buffer, "Hola");

		send_content_with_header(coordinator_socket, DATA, &data, sizeof(InstanceConnection));

		log_info(logger, "Envío saludo al coordinador");

		//Armo estructura para recibir la respuesta y la espero...

		MessageHeader * header = malloc(sizeof(MessageHeader));

		if(recv(coordinator_socket, header, sizeof(MessageHeader), 0) == -1) {
				log_error(logger, "Error al recibir el MessageHeader\n");
				return 1;
			}

		switch((*header).type) {
				case DATA_RECIEVED:
					log_info(logger,"El COORDINADOR me confirma que recibío los datos con éxito");
					fflush(stdout);

					break;
				case UNKNOWN_MSG_TYPE:
					log_error(logger, "El COORDINADOR no reconoció el último mensaje enviado");
					fflush(stdout);

					break;
				default:
					log_error(logger, "No reconozco el tipo de mensaje");
					fflush(stdout);

					int num = 1;
					send_content_with_header(coordinator_socket, UNKNOWN_MSG_TYPE, &num, sizeof(num));

					break;
			}
	return 0;



}
