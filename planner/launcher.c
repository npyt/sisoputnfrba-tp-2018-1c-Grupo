#include "headers.h"


t_log * logger;
t_config * config;
// ESI * incoming_esi;
// PlannerAlgorithm * planner_algorithm;

int main(){

	logger = log_create("planner_logger.log", "PLANNER", true, LOG_LEVEL_TRACE);
	config = config_create("planner_config.cfg");
	//planner_algorithm = config_get_string_value(config, "PLAN_ALG");

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

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, server_socket);

	pthread_exit(NULL);
	close(server_socket);

//	t_queue * ready_queue = queue_create();


//	ESI esi_location;
//	while(1){
//	incoming_esi = recibe();
//	 // Verificar con el ESI lo recibido
//	 //
//	 queue_add(ready,esi_arrive);
//	 switch(planner_algorithm){
//	 case FIFO:
//		 fifo(incoming_esi,incoming_esi->status);
//		 break;
//	 case SJF_SD:
//		 sjfsd(incoming_esi,incoming_esi->status);
//		 break;
//	 case SJF_CD:
//		 sjfcd(incoming_esi,incoming_esi->status);
//		 break;
//	 case HRRN:
//		 hrrn(incoming_esi,incoming_esi->status);
//		 break;
//
//	 }
//	 esi_location = queue_pop(ready_queue);
//	 //consulta a cordinador
//	 change_ESI_status(esi,STATUS_RUNNING);
//	 //run
//	 //verifico
//	 //saco si hace falta
//	}


	return 0;
}

//void fifo(ESI * esi, ESIStatus * status){
//    // Modificación del ESI: en otra func VVVV
//    change_ESI_status(esi, STATUS_READY);
//    int queue_add(ready, EL_ESI);
//}
//
//void change_ESI_status(ESI*esi, ESIStatus * status){
//    esi->status = status;
//}


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

//
//ESI recibe(){
//	//recibimos los esi enviados
//}

