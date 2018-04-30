#include "headers.h"

t_log * logger;
t_config * config;
ESI * incoming_esi;
t_queue * ready_queue;
t_queue * blocked_queue;
t_queue * finished_queue;
t_queue * running_queue;

PlannerAlgorithm * planner_algorithm;


int main(){

	planner_algorithm = FIFO;
	logger = log_create("planner_logger.log", "PLANNER", true, LOG_LEVEL_TRACE);
	config = config_create("planner_config.cfg");
	define_algorithm(config, planner_algorithm);

/*
 * SOCKET
 */

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

/*
 * THREADS
 */

	pthread_t listening_thread_id;
	pthread_t planner_console_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, server_socket);
	pthread_create(&planner_console_id, NULL, planner_console_launcher, NULL);

/*
 * QUEUES
 */

	create_queues();

	/*
	 * Recibir mensaje del ESI
	 * PLANIFICAR > Ordenar en la cola de ready (Posibilidad de cambiar el algoritmo)
	 *
	 *
	 * sort_esi(incoming_esi, planner_algorithm);
	 *
	 * Enviar mensaje de ejecutar al ESI primero de la cola de ready
	 *
	 * Recibir mensaje del Coordinador con qué recursos se bloquean (Pasar de Ejecutar a Bloqueados)
	 *
	 * Recibir mensaje del ESI si ya terminó su proceso (Pasar de Ejecutar a Finished)
	 *
	 * En próximos algoritmos, sacar de la cola de Ready a Listos
	 *
	 */

//	ESI esi_location;
//	while(1){
//	incoming_esi = recibe();
//	 // Verificar con el ESI lo recibido
//	 //
//	 queue_add(ready,esi_arrive);
//
//	 esi_location = queue_pop(ready_queue);
//	 //consulta a cordinador
//	 change_ESI_status(esi,STATUS_RUNNING);
//	 //run
//	 //verifico
//	 //saco si hace falta
//	}




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

		//Procesar el resto del mensaje dependiendo del tipo recibido
		switch((*header).type) {
		}
	}
}

void create_queues(){
	ready_queue = queue_create();
	blocked_queue = queue_create();
	finished_queue = queue_create();
	running_queue = queue_create();
}

void sort_esi(ESI * esi, PlannerAlgorithm * algorithm){
	 switch(algorithm){
	 case FIFO:
		 fifo(incoming_esi);
		 break;
	 case SJF_SD:
		 sjfsd(incoming_esi);
		 break;
	 case SJF_CD:
		 sjfcd(incoming_esi);
		 break;
	 case HRRN:
		 hrrn(incoming_esi);
		 break;
	 }
}


void define_algorithm(t_config * config, PlannerAlgorithm * planner_algorithm){
	char * buffer_algorithm;

	strcpy(buffer_algorithm, config_get_string_value(config, "PLAN_ALG"));

	if(strcmp(buffer_algorithm, "FIFO")){
		planner_algorithm = FIFO;
	}else if(strcmp(buffer_algorithm, "SJF_CD")){
		planner_algorithm = SJF_CD;
	}else if(strcmp(buffer_algorithm, "SJF_SD")){
		planner_algorithm = SJF_SD;
	}else if(strcmp(buffer_algorithm, "HRRN")){
		planner_algorithm = HRRN;
	}else{
		// CASO DE ERROR
	}
}




void fifo(ESI * esi){
    queue_push(ready_queue, esi);
}

void sjfsd(ESI * esi){

}
void sjfcd(ESI * esi){

}
void hrrn(ESI * esi){

}

void change_ESI_status(ESI * esi, ESIStatus * esi_status){
    esi->status = esi_status;
}



//
//ESI recibe(){
//	//recibimos los esi enviados
//}

