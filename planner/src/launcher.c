#include <libgrupo/headers.h>
#include "include/headers.h"

t_log * logger;
t_config * config;

t_queue * ready_queue;
t_queue * blocked_queue;
t_queue * finished_queue;
t_queue * running_queue;
t_list * esi_sockets_list;
PlannerAlgorithm planner_algorithm;
int esi_id_counter;
int superflag = 0;

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
	send_only_header(coordinator_socket, PLANNER_COORD_HANDSHAKE);
	// END COORD CONNECTION

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, server_socket);

	pthread_t planner_console_id;
	pthread_create(&planner_console_id, NULL, planner_console_launcher, NULL);

//	pthread_t running_thread_id;
//	pthread_create(&running_thread_id, NULL, running_thread, NULL);


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
				ESI * esi_registered = malloc(sizeof(ESI));
				register_esi(esi_registered);

				// ========== SEND STRUCTURE REGISTRATION VARIATION ==========
				//send_only_header(client_socket, ESI_PLANNER_HANDSHAKE_OK);
				//ESIRegistration * esi_name = malloc(sizeof(ESIRegistration));
				//strcpy(esi_name->id, esi_registered->id);
				//send_content_with_header(client_socket, ESI_PLANNER_HANDSHAKE_OK, esi_name, sizeof(ESIRegistration));
				//free(esi_name);
				// ========== END SEND STRUCTURE REGISTRATION VARIATION ==========

				char * esi_buffer_name[ESI_NAME_MAX_SIZE];
				strcpy(esi_buffer_name, esi_registered->id);
				//register_esi_socket(client_socket, esi_registered);
				log_info(logger, "[%s_REGISTERED_IN_READY_QUEUE]", esi_registered->id);
				send_content_with_header(client_socket, ESI_PLANNER_HANDSHAKE_OK, esi_buffer_name, sizeof(ESI_NAME_MAX_SIZE));

				sort_esi(esi_registered, planner_algorithm);
				fflush(stdout);
				break;
				/*case OPERATION_ERROR:
				 *	esi_to_finish = queue_pop(running_queue);
				 *	queue_push(finished_queue, esi_to_finish);
				 *	change_esi_status(esi_to_finish, STATUS_FINISHED);
				 *	ejecutar el siguiente
				 *
				*/
			// END TESTING CODE
			case ESI_EXECUTION_OK:
				log_info(logger, "[ESI_EXECUTION_OK]");
				ESI * esi_execution_ok = queue_peek(running_queue);
				esi_execution_ok->last_estimate = estimation(esi_execution_ok->program_counter, esi_execution_ok->last_estimate);
				esi_execution_ok->program_counter++;
				sort_esi(esi_execution_ok, planner_algorithm);
				// Continua ejecutando?
				break;
			case ESI_EXECUTION_FINISHED:
				log_info(logger, "[ESI_EXECUTION_FINISHED]");
				ESI * esi_exe_finished = queue_pop(running_queue);
				change_esi_status(esi_exe_finished, STATUS_FINISHED);
				queue_push(finished_queue, esi_exe_finished);
				break;
			case PLANNER_COORD_HANDSHAKE_OK:
				log_info(logger, "El COORDINADOR aceptó mi conexión");
				fflush(stdout);
				break;
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

void * running_thread(){
	while(1){
		//sort_queues();
		if(1) { //algun tipo de activación del while
			ESI * esi_to_run = queue_pop(ready_queue);
			queue_push(running_queue, esi_to_run);
			change_esi_status(esi_to_run, STATUS_RUNNING);
			log_info(logger, "[%s_NOW_RUNNING]", esi_to_run->id);
			//Abajajo busco socket en lista y envio mensaje de ejecutar
			//int esi_socket = search_esi_socket(esi_to_run);
			//send_only_header(esi_socket, PLANNER_ESI_RUN);
		}
	}
}

//void sort_queues();


//void sorting_thread(PlannerAlgorithm planner_algorithm){
//	while(1){
//		switch(planner_algorithm){
//			 case FIFO:
//				 queue_push(ready_queue, esi);
//				 change_esi_status(esi, STATUS_READY);
//				 break;
//			 case SJF_SD:
//				 break;
//			 case SJF_CD:
//				 break;
//			 case HRRN:
//				 break;
//			 }
//	}
//}



void register_esi_socket(int socket, ESI * esi){
	ESIsocket * esi_socket = (ESIsocket *) malloc(sizeof(ESIsocket));
	strcpy(esi_socket->id, esi->id);
	esi_socket->esi_socket=socket;
	list_add(esi_sockets_list, esi_socket);
}

float estimation(int r_duration, float r_estimation) {
	int alpha = config_get_int_value(config, "ALPHA");
	float estimation = (alpha/100)*r_duration + (1-alpha/100)*r_estimation;
	return estimation;
}

// ========== SEARCH CLOSURE BY ESI SOCKET ==========
//void search_esi_socket(t_list *esi_sockets, ESI * esi) {
//    int _search_esi_by_socket(ESI *p) {
//        return strcmp(esi->id, p->id);
//    }
//	list_find(esi_sockets, _search_esi_by_socket);
//}
// ========== END SEARCH CLOSURE BY ESI SOCKET ==========
void register_esi(ESI * incoming_esi){
	change_esi_status(incoming_esi, STATUS_NEW);
	incoming_esi->last_estimate =config_get_int_value(config, "EST_ZERO");
	incoming_esi->idle_counter = 0;
	incoming_esi->program_counter = 0;
	incoming_esi->last_job = 0;
	assign_esi_id(incoming_esi);
}

void assign_esi_id(ESI * incoming_esi){
	char * buffer_name[ESI_NAME_MAX_SIZE];
	sprintf(buffer_name, "ESI%d", esi_id_counter);
	strcpy(incoming_esi->id, buffer_name);
	esi_id_counter++;
}


void sort_esi(ESI * esi, PlannerAlgorithm algorithm){
	 switch(algorithm){
	 case FIFO:
		 queue_push(ready_queue, esi);
		 change_esi_status(esi, STATUS_READY);
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

void change_esi_status(ESI * esi, ESIStatus esi_status){
    esi->status = esi_status;
}

void create_queues(){
	ready_queue = queue_create();
	blocked_queue = queue_create();
	finished_queue = queue_create();
	running_queue = queue_create();
}

