#include "headers.h"

t_log * logger;
t_config * config;

typedef struct{
	char buffer[100];
} PlannerBuffer;



int main() {
	printf("COORDINATOR");

	logger = log_create("coordinator_logger.log", "COORDINATOR", true, LOG_LEVEL_TRACE);
	config = config_create("coordinator_config.cfg");

	t_list * instances = list_create();

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

	struct sockaddr_in client_addr;
	int c = sizeof(struct sockaddr_in);
	int client_connection = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t *)&c);

	if(client_connection < 0){
			log_error(logger, "ERROR AL ACEPTAR CONEXION");
			return 1;
		}

	log_info(logger, "RECIBI UNA CONEXION Y SU FD DE SOCKET ES: %d\n", client_connection);


	MessageHeader * header = malloc(sizeof(MessageHeader));

	if(recv(client_connection, header, sizeof(MessageHeader), 0) == -1){
			log_error(logger, "ERROR AL RECIBIR LOS DATOS");
			return 1;
		}


	switch((*header).type){

	case PLANNER_DATA: ;

		PlannerBuffer * planner = malloc(sizeof(PlannerBuffer));

		if(recv(client_connection, planner, sizeof(PlannerBuffer), 0) == -1){
				log_error(logger, "ERROR AL RECIBIR DATOS DEL PLANNER");
				return 1;
		}

		log_info(logger, "Recibi: %s", (*planner).buffer);

		int num = 1;
		send_content_with_header(client_connection, DATA_RECIEVED, &num, sizeof(num));

		break;

		case UNKNOWN_MSG_TYPE:

			log_error(logger, "No se reconocio el dato enviado");


		break;

		default:
			log_info(logger, "No reconozco el tipo de mensaje enviado");

			num = 1;
			send_content_with_header(client_connection, UNKNOWN_MSG_TYPE, &num, sizeof(num));

	}

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, server_socket);

	Instance * instance_1 = malloc(sizeof(Instance));
	(*instance_1).algorithm = CIRC;
	(*instance_1).dump = 10;
	(*instance_1).socket = 35;
	(*instance_1).entry_table_fst = NULL;
	strcpy((*instance_1).mounting_point, "/");
	strcpy((*instance_1).name, "Mi Instancia 1");
	create_instance(instances, instance_1);

	Instance * instance_2 = malloc(sizeof(Instance));
	(*instance_2).algorithm = CIRC;
	(*instance_2).dump = 10;
	(*instance_2).socket = 27;
	(*instance_2).entry_table_fst = NULL;
	strcpy((*instance_2).mounting_point, "/");
	strcpy((*instance_2).name, "Mi Instancia 2");
	create_instance(instances, instance_2);

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

		}
	}
}

void * instance_thread(Instance * instance) {

}

void create_instance(t_list * instances, Instance * instance) {
	log_info(logger, "LEVANTADA INSTANCIA %s EN SOCKET %d", (*instance).name, (*instance).socket);
	list_add(instances, instance);

	pthread_t tid;
	pthread_create(&tid, NULL, instance_thread, instance);
}
