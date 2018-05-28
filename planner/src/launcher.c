#include <libgrupo/headers.h>
#include "include/headers.h"

t_log * logger;
t_config * config;
ESI * incoming_esi;
t_queue * ready_queue;
t_queue * blocked_queue;
t_queue * finished_queue;
t_queue * running_queue;
PlannerAlgorithm planner_algorithm;
int esi_id_counter;

int main(){
	// CONFIG
	planner_algorithm = FIFO;
	logger = log_create("planner_logger.log", "PLANNER", true, LOG_LEVEL_TRACE);
	config = config_create("planner_config.cfg");
	esi_id_counter = config_get_int_value(config, "INIT_ID");
	define_planner_algorithm(config, planner_algorithm);
	// END CONFIG

	// QUEUES
	create_queues();
	// END QUEUES

	// CONNECTION SOCKET
	int server_socket = server_start(atoi(config_get_string_value(config, "PORT")));
	if (server_socket == -1) {
		log_error(logger, "ERROR AL INICIAR SERVIDOR");
		return 1;
	} else {
		log_info(logger, "SERVIDOR INICIADO");
	}

	if (listen(server_socket, 5) == -1) {
		log_error(logger, "ERROR AL ESCUCHAR EN PUERTO");
	} else {
		log_info(logger, "SERVIDOR ESCUCHANDO %d", atoi(config_get_string_value(config, "PORT")));
	}
	// END CONNECTION SOCKET

	// COORD CONNECTION
	int coordinator_socket = connect_with_server(config_get_string_value(config, "IP_COORD"),
			atoi(config_get_string_value(config, "PORT_COORD")));
	if (coordinator_socket < 0){
		log_error(logger, " ERROR AL CONECTAR CON EL COORDINADOR");
		return 1;
	} else {
		log_info(logger, " CONECTADO EN: %d", coordinator_socket);
	}
	log_info(logger, "Envío saludo al coordinador");
	MessageHeader * header = malloc(sizeof(MessageHeader));
	send_only_header(coordinator_socket, PLANNER_COORD_HANDSHAKE);
	if (recv(coordinator_socket, header, sizeof(MessageHeader), 0) == -1) {
		log_error(logger, "Error al recibir el MessageHeader\n");
		return 1;
	}
	switch((*header).type){
		case PLANNER_COORD_HANDSHAKE_OK:
			log_info(logger, "El COORDINADOR aceptó mi conexión");
			fflush(stdout);
			break;
	}

	// END COORD CONNECTION

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, server_socket);

	pthread_t planner_console_id;
	pthread_create(&planner_console_id, NULL, planner_console_launcher, NULL);


	pthread_exit(NULL);
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

		switch((*header).type) {
			case ESI_PLANNER_HANDSHAKE:
				log_info(logger, "[INCOMING_CONNECTION_ESI]");
				send_only_header(client_socket, ESI_PLANNER_HANDSHAKE_OK);
				break;
			// TESTING CODE
			case INCOMING_ESI:
				log_info(logger, "[INCOMING_NEW_ESI]");
				ESI * esi_registered = malloc(sizeof(ESI));
				register_esi(esi_registered);
				sort_esi(esi_registered, planner_algorithm);
				break;
			// END TESTING CODE
			case UNKNOWN_MSG_TYPE:
				log_error(logger, "[MY_MESSAGE_HASNT_BEEN_DECODED]");
				break;
			default:
				log_error(logger, "[UNKOWN_MESSAGE_RECIEVED]");
				send_only_header(client_socket, UNKNOWN_MSG_TYPE);
				break;
		}
	}
}

/* // RUN ORDER
 * queue_pop(ready_queue, esi_to_run);
 * queue_push(running_queue, esi_to_run);
 * change_esi_status(esi_to_run, STATUS_RUNNING);
 * send_only_header(client_socket, PLANNER_ESI_RUN);
 * // END RUN ORDER
 */

// TESTING CODE

void register_esi(ESI * incoming_esi){
	change_ESI_status(incoming_esi, STATUS_NEW);
	incoming_esi->last_estimate =config_get_int_value(config, "EST_ZERO");
	incoming_esi->idle_counter = 0;
	incoming_esi->program_counter = 0;
	incoming_esi->last_job = 0;
	assign_esi_id(incoming_esi);
}

void assign_esi_id(ESI * incoming_esi){
	incoming_esi->id = esi_id_counter;
	esi_id_counter++;
}


void sort_esi(ESI * esi, PlannerAlgorithm algorithm){
	 switch(algorithm){
	 case FIFO:
		 queue_push(ready_queue, esi);
		 change_ESI_status(esi, STATUS_READY);
		 break;
	 case SJF_SD:
		 break;
	 case SJF_CD:
		 break;
	 case HRRN:
		 break;
	 }
}
// END TESTING CODE


void define_planner_algorithm(t_config * config, PlannerAlgorithm planner_algorithm){
	char * buffer_algorithm = config_get_string_value(config, "PLAN_ALG");

	if(strcmp(buffer_algorithm, "FIFO") == 0){
		planner_algorithm = FIFO;
	}else if(strcmp(buffer_algorithm, "SJF_CD") == 0){
		planner_algorithm = SJF_CD;
	}else if(strcmp(buffer_algorithm, "SJF_SD") == 0){
		planner_algorithm = SJF_SD;
	}else if(strcmp(buffer_algorithm, "HRRN") == 0){
		planner_algorithm = HRRN;
	}else{
		log_info(logger, " ERROR AL LEER EL ALGORITMO DE PLANIFICACIÓN");
	}
	log_info(logger, "ALGORITMO DE PLANIFICACIÓN: %s", buffer_algorithm);
	free(buffer_algorithm);
}

void change_ESI_status(ESI * esi, ESIStatus esi_status){
    esi->status = esi_status;
}

void create_queues(){
	ready_queue = queue_create();
	blocked_queue = queue_create();
	finished_queue = queue_create();
	running_queue = queue_create();
}

