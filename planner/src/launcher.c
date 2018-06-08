#include <libgrupo/headers.h>
#include "headers.h"

t_log * logger;
t_config * config;

t_list * taken_keys;
t_list * ready_queue;
t_list * blocked_queue;
t_list * finished_queue;
t_list * running_queue;
t_list * esi_sockets_list;
PlannerAlgorithm planner_algorithm;
int esi_id_counter;
int superflag = 0;

int main(){
	// CONFIG
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

	// LISTENING THREAD
	SocketToListen * sockets_to_listen = malloc(sizeof(SocketToListen));
	sockets_to_listen->coord_socket = coordinator_socket;
	sockets_to_listen->server_socket = server_socket;
	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_threads, sockets_to_listen);
	// END LISTENING THREAD

	pthread_t planner_console_id;
	pthread_create(&planner_console_id, NULL, planner_console_launcher, NULL);

	pthread_t running_thread_id;
	pthread_create(&running_thread_id, NULL, running_thread, NULL);


	pthread_exit(NULL);
	close(server_socket);

	return 0;
}

void * listening_threads(SocketToListen * socket_to_listen) {
	int coordinator_socket = socket_to_listen->coord_socket;
	int server_socket = socket_to_listen->server_socket;
	fd_set master;
	fd_set read_fds;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(coordinator_socket, &master);
	FD_SET(server_socket, &master);
	int new_client_socket;
	int fdmax = server_socket>coordinator_socket? server_socket : coordinator_socket;

	while(1) {
		read_fds = master;
		select(fdmax+1, &read_fds, NULL, NULL, NULL);


		for(int i = 0; i <= fdmax; i++){
			if (FD_ISSET(i, &read_fds)){
				if(i==server_socket){
					struct sockaddr_in client;
					int c = sizeof(struct sockaddr_in);
					new_client_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t *)&c);
					FD_SET(new_client_socket, &master);
					if(new_client_socket > fdmax){
						fdmax = new_client_socket;
					}
					log_info(logger, "CONEXION RECIBIDA EN SOCKET %d", new_client_socket);
				}
				else{
					MessageHeader * header = malloc(sizeof(MessageHeader));
					if(recv(i, header, sizeof(MessageHeader), 0) > 0){
						if(FD_ISSET(i, &master)){
							switch((*header).type) {
								case ESI_PLANNER_HANDSHAKE:
									log_info(logger, "[INCOMING_CONNECTION_ESI]");
									ESI * esi_registered = malloc(sizeof(ESI));
									register_esi(esi_registered);
									register_esi_socket(i, esi_registered->id);

									// ========== SEND STRUCTURE REGISTRATION VARIATION ==========
									//send_only_header(client_socket, ESI_PLANNER_HANDSHAKE_OK);
									//ESIRegistration * esi_name = malloc(sizeof(ESIRegistration));
									//strcpy(esi_name->id, esi_registered->id);
									//send_content_with_header(client_socket, ESI_PLANNER_HANDSHAKE_OK, esi_name, sizeof(ESIRegistration));
									//free(esi_name);
									// ========== END SEND STRUCTURE REGISTRATION VARIATION ==========

									char * esi_buffer_name[ESI_NAME_MAX_SIZE];
									strcpy(esi_buffer_name, esi_registered->id);
									list_add_in_index(ready_queue, 0, esi_registered);
									change_esi_status(esi_registered, STATUS_READY);
									log_info(logger, "[%s_REGISTERED_IN_READY_QUEUE]", esi_registered->id);
									send_content_with_header(i, ESI_PLANNER_HANDSHAKE_OK, esi_buffer_name, sizeof(ESI_NAME_MAX_SIZE));

									fflush(stdout);
									break;
								case ESI_EXECUTION_LINE_OK:
									log_info(logger, "[ESI_EXECUTION_OK]");
									ESI * esi_execution_ok = list_get(running_queue, 0);
									esi_execution_ok->last_estimate--;
									esi_execution_ok->program_counter++;
									int esi_socket = search_esi_socket(esi_sockets_list, esi_execution_ok);
									send_only_header(esi_socket, PLANNER_ESI_RUN);
									break;
								case ESI_EXECUTION_FINISHED:
									log_info(logger, "[ESI_EXECUTION_FINISHED]");
									ESI * esi_exe_finished = list_remove(running_queue, 0);
									change_esi_status(esi_exe_finished, STATUS_FINISHED);
									list_add(finished_queue,esi_exe_finished);
									close(i);
									break;
								case PLANNER_COORD_HANDSHAKE_OK:
									log_info(logger, "El COORDINADOR aceptó mi conexión");
									fflush(stdout);
									break;
								case UNKNOWN_MSG_TYPE:
									log_error(logger, "[MY_MESSAGE_HASNT_BEEN_DECODED]");
									break;
								//GUARDA
								case RESOURCE_STATUS_CHANGE_TO_PLANNER:
									{
										ResourceAllocation * check = malloc(sizeof(ResourceAllocation));
										recv(coordinator_socket, check, sizeof(ResourceAllocation), 0);
										change_key_status(check);
									}
									break;
								case CAN_ESI_GET_KEY:
									log_info(logger, "[COORDINATOR_ASKING_FOR_PERMISSION]");

									{
										CoordinatorPlannerCheck * check = malloc(sizeof(CoordinatorPlannerCheck));
										recv(coordinator_socket, check, sizeof(CoordinatorPlannerCheck), 0);

										if(!check_Key_availability(check->key)){
											log_info(logger, "[ALLOW_OP]");
											send_only_header(i, PLANNER_COORDINATOR_OP_OK);
										}else{
											log_info(logger, "[DENY_OP]");
											send_only_header(i, PLANNER_COORDINATOR_OP_FAILED);
											block_esi(check->ESIName);//hacer
										}
										free(check);
									}

									break;
								case CAN_ESI_SET_KEY:
									log_info(logger, "[COORDINATOR_ASKING_FOR_PERMISSION]");

									{
										CoordinatorPlannerCheck * check = malloc(sizeof(CoordinatorPlannerCheck));
										recv(coordinator_socket, check, sizeof(CoordinatorPlannerCheck), 0);

										if(!check_Key_taken(check->key,check->ESIName)){
											log_info(logger, "[ALLOW_OP]");
											send_only_header(i, PLANNER_COORDINATOR_OP_OK);
										}else{
											log_info(logger, "[DENY_OP]");
											send_only_header(i, PLANNER_COORDINATOR_OP_FAILED);
										}
										free(check);
									}

									break;
								case CAN_ESI_STORE_KEY:
									log_info(logger, "[COORDINATOR_ASKING_FOR_PERMISSION]");

									{
										CoordinatorPlannerCheck * check = malloc(sizeof(CoordinatorPlannerCheck));
										recv(coordinator_socket, check, sizeof(CoordinatorPlannerCheck), 0);

										if(!check_Key_taken(check->key,check->ESIName)){
											log_info(logger, "[ALLOW_OP]");
											send_only_header(i, PLANNER_COORDINATOR_OP_OK);
										}else{
											log_info(logger, "[DENY_OP]");
											send_only_header(i, PLANNER_COORDINATOR_OP_FAILED);
										}
										free(check);
									}

									break;
								default:
									log_error(logger, "[UNKOWN_MESSAGE_RECIEVED][%d]", i);
									send_only_header(i, UNKNOWN_MSG_TYPE);
									break;
							}
						}
					}else{
						log_info(logger, "[ERROR_RECV_LOST_SOCKET_%d]", i);
						close(i);
						FD_CLR(i, &master);
						fflush(stdout);
					}
				}
			}
		}
	}
}

void * running_thread(){
	while(1){
		sorting_queues();
		if(!list_is_empty(ready_queue) && list_is_empty(running_queue)) { // Buscar otro tipo de activación del while
			ESI * esi_to_run = list_remove(ready_queue, 0);
			list_add_in_index(running_queue, 0, esi_to_run);
			change_esi_status(esi_to_run, STATUS_RUNNING);
			log_info(logger, "[%s_NOW_RUNNING]", esi_to_run->id);
			int esi_socket = search_esi_socket(esi_sockets_list, esi_to_run);
			send_only_header(esi_socket, PLANNER_ESI_RUN);
		}
	}
}

//void sort_queues();


// ========== SORT BY SENTENCE ESTIMATION ==========
void sort_by_burst() {
    int _sort_burst_esi(ESI *one, ESI *two) {
        return (one->last_estimate<two->last_estimate);
    }
    list_sort(ready_queue, (void*)_sort_burst_esi);
}
// ========== END SORT BY SENTENCE ESTIMATION ==========

void sorting_queues(){
	 ESI * temp_esi_running = malloc(sizeof(ESI));
	 ESI * temp_esi_ready = malloc(sizeof(ESI));

	switch(planner_algorithm){
		 case FIFO:
			 break;
		 case SJF_SD:
			 if(list_size(ready_queue)>1){
				 sort_by_burst();
			 }
			 break;
		 case SJF_CD:
			 temp_esi_running = list_get(running_queue, 0);
			 temp_esi_ready = list_get(ready_queue, 0);
			 sort_by_burst();
			 if(temp_esi_running->last_estimate>temp_esi_ready->last_estimate){
				 list_add(ready_queue,temp_esi_running);
				 list_remove(running_queue, 0);
				 sort_by_burst();
			 }
			 break;
		 case HRRN:
			 break;
	 }

	free(temp_esi_running);
	free(temp_esi_ready);
}



void register_esi_socket(int socket, char * esi_id){
	ESIsocket * esi_socket = malloc(sizeof(ESIsocket));
	strcpy(esi_socket->id, esi_id);
	esi_socket->esi_socket=socket;
	list_add(esi_sockets_list, esi_socket);
}

float estimation(int r_duration, float r_estimation) {
	int alpha = config_get_int_value(config, "ALPHA");
	float estimation = (alpha/100)*r_duration + (1-alpha/100)*r_estimation;
	return estimation;
}

// ========== SEARCH CLOSURE BY ESI SOCKET ==========
int search_esi_socket(t_list *esi_sockets, ESI * esi) {
    int _search_esi_by_socket(ESIsocket *p) {
        return string_equals_ignore_case(esi->id, p->id);
    }
	ESIsocket * esi_socket_found = list_find(esi_sockets, (void*)_search_esi_by_socket);
	return esi_socket_found->esi_socket;
}
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
	ready_queue = list_create();
	blocked_queue = list_create();
	finished_queue = list_create();
	running_queue = list_create();
	esi_sockets_list = list_create();
	taken_keys = list_create();
}



//---MEC place---

/*llega struct (rama esta haciendo receptor.)*/
/*recibo la estructura del cordi:
 *
 typedef struct {
	char key[RESOURCE_KEY_MAX_SIZE];
	InstructionOperation operation;
} InstructionFromCoord;
 *
 * */


//struct de lista_keys_tomadas:
/*typedef struct {
	char key[RESOURCE_KEY_MAX_SIZE];
	char holding_esi_id[RESOURCE_KEY_MAX_SIZE];
} TakenKey;
*/


//precarga de keys bloqueadas:
void pre_load_Blocked_keys(){
	int sizeAr = array_size(config_get_array_value(config, "BLOCKED_KEYS"));
	char* pre_loaded_keys[sizeAr];
	memcpy ( pre_loaded_keys, config_get_array_value(config, "BLOCKED_KEYS") ,sizeAr );
	pload_Keys(pre_loaded_keys,sizeAr);

}

int array_size(char* array[]){
	int sizeA=0;
	int i = 0;
	while(array[i] != NULL){
		i++;
		sizeA++;
	}
	return sizeA+1;
}




//carga y liberacion de keys a lista:
void load_key(char* key,char* esi_id){
	ResourceAllocation*node = malloc(sizeof(ResourceAllocation));
	strcpy(node->key,key);
	strcpy(node->ESIName,esi_id);
	list_add(taken_keys, node);
}




void release_key(char* key,char* esi_id){
	bool key_search(ResourceAllocation*node){
		if(strcmp(node->key,key) == 0){
					return true;
				}else{
					return false;
				}
	}
	list_remove_by_condition(taken_keys,(void*)key_search);
}

void pload_Keys(char* k_array[],int sizeK){
	while(sizeK != 0){
		load_key(k_array[sizeK],NULL);
		sizeK--;
	}
}





bool check_Key_availability(char* key_name){
	bool key_search(ResourceAllocation*node){
		if(strcmp(node->key,key_name) == 0){
			return true;
		}else{
			return false;
		}
	}
	return list_any_satisfy(taken_keys,(void*)key_search); //revisar segundo argumento
}

bool check_Key_taken(char* key,char* esi_id){
	bool key_search(ResourceAllocation*node){
		if(strcmp(node->key,key) == 0){
					return true;
				}else{
					return false;
				}
	}
	ResourceAllocation*nodeK = list_find(taken_keys,(void*)key_search);
	return strcmp(nodeK->ESIName,esi_id);
}


void block_esi(char* ESIName){
	ESI * temp_esi_running = malloc(sizeof(ESI));
	bool esi_search(ESI*node){
			if(strcmp(node->id,ESIName) == 0){
						return true;
					}else{
						return false;
					}
		}
	temp_esi_running = list_find(running_queue,(void*)esi_search);
	list_add(blocked_queue,temp_esi_running);
	list_remove_by_condition(running_queue,(void*)esi_search);
}

//funcion para manejar estado de keys
//recibe lo enviado por coordinador.

void change_key_status(ResourceAllocation*check){
	if(check->status == BLOCKED){
		load_key(check->key,check->ESIName);
	}else{
		release_key(check->key,check->ESIName);
	}

}





