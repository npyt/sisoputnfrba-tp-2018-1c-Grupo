#include <chucknorris/allheaders.h>
#include "console.h"

//Init
t_config * config;
t_log * logger;
t_list * allocations;
t_list * ready_queue;
t_list * blocked_queue;
ESIRegistration * running_esi;
PlannerConfig settings;

//Flags
int ESI_name_counter;
int running_now;

void * listening_thread(int server_socket);
void * running_thread(int a);
void * w_thread(int a);
float ratio(ESIRegistration * esi);
t_list* map_list_for_hrrn();


int main(int argc, char **argv) {
	if(argv[1] == NULL) {
		//exit_with_message("No especificó el archivo de configuración.", EXIT_FAILURE);
		argv[1] = malloc(sizeof(char) * 1024);
		strcpy(argv[1], "config.cfg");
	}

	// Creating log and config files
	config = config_create(argv[1]);
	logger = log_create("log.log", "PLANNER", false, LOG_LEVEL_TRACE);

	// Reading config file
	settings.port = config_get_int_value(config, "PORT");
	settings.alpha = config_get_double_value(config, "ALPHA");
	settings.init_est = config_get_int_value(config, "INIT_EST");
	strcpy(settings.coord_ip, config_get_string_value(config, "COORD_IP"));
	settings.coord_port = config_get_int_value(config, "COORD_PORT");
	allocations = list_create();
	load_preblocked_keys();
	char * buffer = malloc(128 * sizeof(char));
	strcpy(buffer, config_get_string_value(config, "PLAN_ALG"));
	if(strcmp(buffer, "SJF-CD") == 0) {
		settings.planning_alg = SJF_CD;
	} else if(strcmp(buffer, "SJF-SD") == 0) {
		settings.planning_alg = SJF_SD;
	} else if(strcmp(buffer, "HRRN") == 0) {
		settings.planning_alg = HRRN;
	} else {
		settings.planning_alg = FIFO;
	}
	free(buffer);

	// First flag and queues inits
	ESI_name_counter = 0;
	running_esi = NULL;
	running_now = 0;
	ready_queue = list_create();
	blocked_queue = list_create();

	// Starting Server Socket
	int my_socket = prepare_socket_in_port(settings.port);
	if (my_socket == -1) {
		print_and_log_trace(logger, "[FAILED_TO_OPEN_PLANNER_PORT]");
		exit(EXIT_FAILURE);
	}

	// Threads
	pthread_t running_thread_id;
	pthread_create(&running_thread_id, NULL, running_thread, NULL);

	pthread_t listening_thread_id;
	pthread_create(&listening_thread_id, NULL, listening_thread, my_socket);

	pthread_t console_thread_id;
	pthread_create(&console_thread_id, NULL, planner_console_launcher, NULL);

//	// Test thread
//	pthread_t w_thread_id;
//	pthread_create(&w_thread_id, NULL, w_thread, my_socket);

	// Exit
	pthread_exit(NULL);
	return EXIT_SUCCESS;
}

void * w_thread(int a){
	while(1){
		sleep(2);
		print_and_log_debug(logger, "STATUS");
		for(int a=0 ; a<allocations->elements_count ; a++) {
			ResourceAllocation * ra = list_get(allocations, a);
			print_and_log_debug(logger, "  TY %d %s %d", ra->esi_id, ra->key, ra->type);
		}
		print_and_log_debug(logger, "STATUS");
	}
}

void * listening_thread(int server_socket) {
	int clients[MAX_SERVER_CLIENTS];
	int a, max_sd, activity_socket, incoming_socket, esi_f_id;
	fd_set master_set;
	int coordinator_socket = connect_with_server(settings.coord_ip, settings.coord_port);

	for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
		clients[a] = 0;
	}
	clients[0] = server_socket;
	clients[1] = coordinator_socket;

	if(listen(server_socket, MAX_SERVER_CLIENTS) == -1) {
		print_and_log_trace(logger, "[FAILED_TO_OPEN_LISTEN_AT_PORT]");
		exit(EXIT_FAILURE);
	}

	print_and_log_trace(logger, "[READY_FOR_CONNECTIONS]");

	send_message_type(coordinator_socket, HSK_PLANNER_COORD);
	print_and_log_trace(logger, "[SAYING_HI_TO_COORDINATOR]");

	while(1) {
		FD_ZERO(&master_set);
		FD_SET(server_socket, &master_set);
		max_sd = server_socket;
		for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
			if(clients[a] > 0) {
				FD_SET(clients[a], &master_set);
				if(max_sd < clients[a]) {
					max_sd = clients[a];
				}
			}
		}

		activity_socket = select(max_sd + 1, &master_set, NULL, NULL, NULL);

		if(FD_ISSET(server_socket, &master_set)) {
			incoming_socket = server_socket;

			struct sockaddr_in client;
			int c = sizeof(struct sockaddr_in);
			incoming_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t *)&c);

			//From listen

			print_and_log_trace(logger, "[INCOMING_CONNECTION]", incoming_socket);

			for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
				if(clients[a] == 0) {
					clients[a] = incoming_socket;
					break;
				}
			}


		}
		for(a=0 ; a<MAX_SERVER_CLIENTS ; a++) {
			if(FD_ISSET(clients[a], &master_set)) {
				incoming_socket = clients[a];
				//From another client
				//Recieve Header
				MessageHeader * i_header = malloc(sizeof(MessageHeader));
				if (recieve_header(incoming_socket, i_header) <= 0 ) {
					//Disconnected


					clients[a] = 0;
					print_and_log_trace(logger, "[SOCKET_DISCONNECTED]");
				} else {
					//New message

					MessageHeader * header = malloc(sizeof(MessageHeader));

					switch(i_header->type) {
						case TEST:
							//printf("Test Recieved\n");
							fflush(stdout);
							break;
						case HSK_PLANNER_COORD_OK:
							print_and_log_trace(logger, "[COORDINATOR_SAYS_HI]");
							break;
						case HSK_ESI_PLANNER:
							print_and_log_trace(logger, "[NEW_ESI_CONNECTION]");

							// Registrate ESI
							ESIRegistration * re = malloc(sizeof(ESIRegistration));
							re->esi_id = ESI_name_counter;
							re->socket = incoming_socket;
							re->rerun_last_instruction = 0;
							re->kill_on_next_run = 0;
							re->status = S_READY;
							re->estimation= settings.init_est;
							re->waiting_counter=0;
							re->job_counter=0;
							re->response_ratio=0;
							list_add(ready_queue, re);

							// Response
							header->type = HSK_ESI_PLANNER_OK;
							send_header(incoming_socket, header);
							send_data(incoming_socket, &ESI_name_counter, sizeof(int));
							ESI_name_counter++;

							print_and_log_trace(logger, "[ASSIGNED_ID_%d]", re->esi_id);
							print_and_log_trace(logger, "[ASSIGNED_ESTIMATION_%f]", re->estimation);
							break;
						case INSTRUCTION_PERMISSION:
							if(running_esi->kill_on_next_run == 1){
								running_esi = NULL;
								running_now = 0;
								finish_esi(running_esi->esi_id);
							} else {
								print_and_log_trace(logger, "[COORDINATOR_ASKS_FOR_PERMISSION_TO_EXECUTE]");
								InstructionDetail * instruction = malloc(sizeof(InstructionDetail));
								recieve_data(incoming_socket, instruction, sizeof(InstructionDetail));

								print_instruction(instruction);

								switch (instruction->type) {
									case GET_OP:
										if(is_key_free(instruction->key)) {
											print_and_log_trace(logger, "[ALLOW_OPERATION]");
											send_message_type(incoming_socket, INSTRUCTION_ALLOWED);
										} else {
											print_and_log_trace(logger, "[BLOCK_OPERATION]");
											send_message_type(incoming_socket, INSTRUCTION_NOT_ALLOWED);

											if(running_esi->esi_id == instruction->esi_id) {
												print_and_log_trace(logger, "[BLOCKING_ESI_%d]", running_esi->esi_id);

												ResourceAllocation * ra = malloc(sizeof(ResourceAllocation));
												ra->esi_id = running_esi->esi_id;
												strcpy(ra->key, instruction->key);
												ra->type = WAITING;
												process_allocation(ra);

												running_esi->status = S_BLOCKED;
												running_esi->rerun_last_instruction = 1;
												add_esi_to_blocked(running_esi);
												running_esi = NULL;

												running_now = 0;
											}
										}

										break;
									case SET_OP:
										if(key_exists(instruction->key)) {
											if(is_key_allocated_by(instruction->key, instruction->esi_id)) {
												print_and_log_trace(logger, "[ALLOW_OPERATION]");
												send_message_type(incoming_socket, INSTRUCTION_ALLOWED);
											} else {
												print_and_log_trace(logger, "[BLOCK_OPERATION]");
												send_message_type(incoming_socket, INSTRUCTION_NOT_ALLOWED);
											}
										} else {
											print_and_log_trace(logger, "[BLOCK_OPERATION][COMMAND_ESI_ABORTION]");
											send_message_type(incoming_socket, INSTRUCTION_NOT_ALLOWED_AND_ABORT);
										}


										break;
									case STORE_OP:
										if(key_exists(instruction->key)) {
											if(is_key_allocated_by(instruction->key, instruction->esi_id)) {
												print_and_log_trace(logger, "[ALLOW_OPERATION]");
												send_message_type(incoming_socket, INSTRUCTION_ALLOWED);
											} else {
												print_and_log_trace(logger, "[BLOCK_OPERATION]");
												send_message_type(incoming_socket, INSTRUCTION_NOT_ALLOWED);
											}
										} else {
											print_and_log_trace(logger, "[BLOCK_OPERATION][COMMAND_ESI_ABORTION]");
											send_message_type(incoming_socket, INSTRUCTION_NOT_ALLOWED_AND_ABORT);
										}

										break;
								}
							}
							break;
						case NEW_RESOURCE_ALLOCATION:
							print_and_log_trace(logger, "[INCOMING_RESOURCE_ALLOCATION]");

							ResourceAllocation * ra = malloc(sizeof(ResourceAllocation));
							recieve_data(incoming_socket, ra, sizeof(ResourceAllocation));

							process_allocation(ra);
							free(ra);
							break;
						case INSTRUCTION_OK_TO_PLANNER:
							print_and_log_trace(logger, "[CURRENT_INSTRUCTION_SUCCESSFULLY_EXECUTED]");

							running_esi->rerun_last_instruction = 0;
							running_now = 0;

							// Subtract estimation
							if(running_esi->estimation)
								running_esi->estimation--;

							print_and_log_trace(logger, "[JOB_COUNTER][%d]", running_esi->job_counter);
							print_and_log_trace(logger, "[ESTIMATION][%f]", running_esi->estimation);

							// Prepare ratio for HRRN
							if(settings.planning_alg == HRRN)
								ready_queue = map_list_for_hrrn();
							break;
						case ESI_FINISHED:
							print_and_log_trace(logger, "[ESI_SAYS_ITS_DONE]");
							recieve_data(incoming_socket, &esi_f_id, sizeof(int));
							print_and_log_trace(logger, "[ESI_ID_%d]", esi_f_id);
							finish_esi(esi_f_id);
							running_esi = NULL;
							running_now = 0;
							break;
						case ESI_FINISHED_BY_ERROR:
							recieve_data(incoming_socket, &esi_f_id, sizeof(int));
							print_and_log_trace(logger, "[ESI_FAILURE]");
							finish_esi(esi_f_id);
							running_esi = NULL;
							running_now = 0;
							break;

					}

					free(header);

				}
				free(i_header);
			}
		}
	}
}

void * running_thread(int a) {
	while(1) {
		// Sort
		sort_queues();
		// Verify running state
		if (get_running_flag()) {
			if(running_esi != NULL && !running_now) {
				// Run ESI in running
				print_and_log_trace(logger, "[ESI_WILL_EXECUTE][%d]", running_esi->esi_id);
				running_esi->job_counter++;
				if(running_esi->rerun_last_instruction) {
					send_message_type(running_esi->socket, EXECUTE_PREV_INSTRUCTION);
				} else {
					send_message_type(running_esi->socket, EXECUTE_NEXT_INSTRUCTION);
				}
				running_now = 1;
			}
		}
	}
}

void add_esi_to_blocked(ESIRegistration * esi) {
	list_add(blocked_queue, esi);
}

void load_preblocked_keys(){
	int count = array_size(config_get_array_value(config, "BLOCKED_KEYS")) - 1;
	char ** pre_loaded_keys = config_get_array_value(config, "BLOCKED_KEYS");
	for(int b=0 ; b<count ; b++){
		block_resource_with_esi(pre_loaded_keys[b], -1);
	}
}

int array_size(char* array[]){
	int size=0;
	int i = 0;
	while(array[i] != NULL){
		i++;
		size++;
	}
	return size+1;
}

int is_key_free(char key[KEY_NAME_MAX]) {
	return get_owner_esi(key) == NULL;
}

int is_key_allocated_by(char key[KEY_NAME_MAX], int esi_id) {
	return get_owner_esi(key) == esi_id;
}

void process_allocation(ResourceAllocation * ra) {
	switch(ra->type) {
		case BLOCKED:
			block_resource_with_esi(ra->key, ra->esi_id);
			break;
		case WAITING:
			list_add(allocations, ra);
			break;
		case RELEASED:
			unlock_resource_with_esi(ra->key, ra->esi_id);
			break;
	}
}

void block_resource_with_esi(char key[KEY_NAME_MAX], int esi_id) {
	ResourceAllocation * ra = malloc(sizeof(ResourceAllocation));
	ra->esi_id = esi_id;
	ra->type = BLOCKED;
	strcpy(ra->key, key);

	list_add(allocations, ra);
	print_and_log_trace(logger, "[BLOCKED_KEY][%s][ESI_%d]", key, ra->esi_id);
}

//void unlock_resource_with_esi(char key[KEY_NAME_MAX], int esi_id){
//	bool _by_resource_taken_by_esi(ResourceAllocation * ra){
//		if(strcmp(ra->key, key) == 0 && ra->esi_id == esi_id && ra->type == BLOCKED){
//			print_and_log_trace(logger, "[RELEASED_KEY][%s][ESI_%d]", key, ra->esi_id);
//			free_esis_waiting_for(key);
//			return false;
//		}else{
//			return true;
//		}
//	}
//	allocations = list_filter(allocations, (void*)_by_resource_taken_by_esi);
//}

void unlock_resource_with_esi(char key[KEY_NAME_MAX], int esi_id) {
	int a;
	for(a=0 ; a<allocations->elements_count ; a++) {
		ResourceAllocation * ra = list_get(allocations, a);
		if(strcmp(ra->key, key) == 0 &&
				ra->esi_id == esi_id &&
				ra->type == BLOCKED) {
			list_remove(allocations, a);
			print_and_log_trace(logger, "[RELEASED_KEY][%s][ESI_%d]", key, ra->esi_id);
			free_esis_waiting_for(key);
			return;
		}
	}
}

void unlock_resource_all(char key[KEY_NAME_MAX]) {
	int a;
	for(a=0 ; a<allocations->elements_count ; a++) {
		ResourceAllocation * ra = list_get(allocations, a);
		if(strcmp(ra->key, key) == 0 &&
				ra->type == BLOCKED) {
			list_remove(allocations, a);
			print_and_log_trace(logger, "[BLOCKED_KEY][%s][ESI_%d]", key, ra->esi_id);
			free_esis_waiting_for(key);
			return;
		}
	}
}

//void free_esis_waiting_for(char key[KEY_NAME_MAX]){
//	bool _by_key_unlocked(ResourceAllocation * ra){
//		if(strcmp(ra->key, key) == 0 && ra->type == WAITING){
//			print_and_log_trace(logger, "[NO_LONGER_WAITING][%s][ESI_%d]", key, ra->esi_id);
//			return false;
//		}else{
//			return true;
//		}
//	}
//	allocations = list_filter(allocations, (void*)_by_key_unlocked);
//}

void free_esis_waiting_for(char key[KEY_NAME_MAX]) {
	fflush(stdout);
	int a;
	for(a=allocations->elements_count-1 ; 0<=a ; a--) {
		ResourceAllocation * ra = list_get(allocations, a);
		if(strcmp(ra->key, key) == 0 &&
				ra->type == WAITING) {
			print_and_log_trace(logger, "[NO_LONGER_WAITING][%s][ESI_%d]", key, ra->esi_id);
			list_remove(allocations, a);
		}
	}
}

//int key_exists(char key[KEY_NAME_MAX]){
//	bool _by_key_existence(ResourceAllocation * ra){
//		return strcmp(ra->key, key) == 0;
//	}
//	return list_any_satisfy(allocations, (void*)_by_key_existence);
//}

int key_exists(char key[KEY_NAME_MAX]) {
	int a;
	for(a=allocations->elements_count-1 ; 0<=a ; a--) {
		ResourceAllocation * ra = list_get(allocations, a);
		if(strcmp(ra->key, key) == 0) return 1;
	}
	return 0;
}

//int get_owner_esi(char key[KEY_NAME_MAX]){
//	bool _by_owner_esi(ResourceAllocation * ra){
//		return strcmp(ra->key, key) == 0 && ra->type == BLOCKED;
//	}
//	ResourceAllocation * ra = list_find(allocations, (void*)_by_owner_esi);
//	return ra->esi_id;
//}

int get_owner_esi(char key[KEY_NAME_MAX]){
	int a;
	for(a=allocations->elements_count-1 ; 0<=a ; a--) {
		ResourceAllocation * ra = list_get(allocations, a);
		if(strcmp(ra->key, key) == 0 &&
				ra->type == BLOCKED) {
			return ra->esi_id;
		}
	}
	return NULL;
}

/*
 * FOR CONSOLE USES
 */
ESIRegistration * search_esi(int esi_id) {
	int a;
	if(running_esi != NULL) {
		if(running_esi->esi_id == esi_id) {
			return running_esi;
		}
	}
	for(a=0 ; a<blocked_queue->elements_count ; a++) {
		ESIRegistration * re = list_get(blocked_queue, a);
		if(re->esi_id == esi_id) {
			return re;
		}
	}
	for(a=0 ; a<ready_queue->elements_count ; a++) {
		ESIRegistration * re = list_get(ready_queue, a);
		if(re->esi_id == esi_id) {
			return re;
		}
	}
	return NULL;
}

//t_list * search_esis_waiting_for(char key[KEY_NAME_MAX]){
//	bool _by_waiting_key(ResourceAllocation * ra){
//		return strcmp(ra->key, key) == 0 && ra->type == WAITING;
//	}
//	return list_filter(allocations, (void*)_by_waiting_key);
//}

t_list * search_esis_waiting_for(char key[KEY_NAME_MAX]) {
	int a;
	t_list * list = list_create();
	for(a=allocations->elements_count-1 ; 0<=a ; a--) {
		ResourceAllocation * ra = list_get(allocations, a);
		if(strcmp(ra->key, key) == 0 &&
				ra->type == WAITING) {
			list_add(list, ra);
		}
	}
	return list;
}

int is_esi_waiting(int esi_id){
	/*No todos los ESI de la blocked queue fueron bloqueados
	 * por estar esperando un recurso ya que tambien se los puede
	 * bloquear por consola
	 */
	int _is_esi_waiting(ResourceAllocation * some_allocation){
		return some_allocation->esi_id == esi_id &&
				some_allocation->type == WAITING;
	}
	return list_any_satisfy(allocations, (void*)_is_esi_waiting);
}

t_list * get_by_allocation_type(AllocationType allocation_type){
	bool _filter_by_allocation(ResourceAllocation * ra){
		return ra->type == allocation_type;
	}
	return list_filter(allocations, (void*)_filter_by_allocation);
}

t_list * get_allocations(){
	return allocations;
}

/*
 * ESTIMATION
 */
float estimate(ESIRegistration*esi){
	float alpha = settings.alpha;
	float estimation = (alpha/100)*(esi->job_counter) + (1-(alpha/100))*(esi->estimation);
	return estimation;
}

/*
 * RATIO
 */
t_list* map_list_for_hrrn() {
    ESIRegistration* _map_list_hrrn_info(ESIRegistration *one) {
    	one->waiting_counter++;
    	one->response_ratio=ratio(one);
        return one;
    }
    return list_map(ready_queue, (void*)_map_list_hrrn_info);
}

float ratio(ESIRegistration * esi){
	return (esi->waiting_counter + esi->estimation) / esi->estimation;
}

void sort_by_ratio() {
    int _sort_ratio_esi(ESIRegistration *one, ESIRegistration *two) {
        return (one->response_ratio>two->response_ratio);
    }
    list_sort(ready_queue, (void*)_sort_ratio_esi);
}

/*
 * BURST
 */
void sort_by_burst() {
    int _sort_burst_esi(ESIRegistration * one, ESIRegistration * two) {
        return (one->estimation<two->estimation);
    }
    if(list_size(ready_queue)>0)
    	list_sort(ready_queue, (void*)_sort_burst_esi);
}

/*
 * SORT
 */
void sort_queues() {
	int a, b;
	//At the end of the function, running_esi must be allocated

	//Looking for free ESIs
	for(a=0 ; a<blocked_queue->elements_count ; a++) {
		ESIRegistration * re = list_get(blocked_queue, a);
		re->status = S_READY;
		for(b=0 ; b<allocations->elements_count ; b++) {
			ResourceAllocation * ra = list_get(allocations, b);
			if(re->esi_id == ra->esi_id && ra->type == WAITING) {
				re->status = S_BLOCKED;
			}
		}
		if(re->status == S_READY) {
			re->estimation = estimate(re);
			print_and_log_trace(logger, "[NEW_ESTIMATION][%f]", re->estimation);
			list_add(ready_queue, list_remove(blocked_queue, a));
		}
	}
	// Switch between planner algorithms
	switch(settings.planning_alg) {
		case FIFO: //FIFO Alg, the queue remains like it was built
			break;
		case SJF_CD: //SJF CD Alg, the queue is sorted by job burst with deallocation
			if(running_esi){
				list_add(ready_queue, running_esi);
				running_esi = NULL;
				sort_by_burst();
			}
			break;
		case SJF_SD: //SJF SD Alg, the queue is sorted by job burst without deallocation
			sort_by_burst();
			break;
		case HRRN: //HRRN Alg, the queue is sorted by response ratio without deallocation
			sort_by_ratio();
			break;
	}
	// Run the first in ready queue
	if(!list_is_empty(ready_queue) && running_esi == NULL) {
		running_esi = list_remove(ready_queue, 0);
		running_esi->status = S_RUNNING;
	}
}

/*
 * FINISH
 */
t_list * delete_allocations_by_id(int esi_id){
	bool _filter_by_name(ResourceAllocation * ra){
		return ra->esi_id != esi_id;
	}
	return list_filter(allocations, (void*)_filter_by_name);
}

void remove_esi_allocations(int esi_id){
	allocations = delete_allocations_by_id(esi_id);
}

void finish_esi(int esi_id){
	remove_esi_allocations(esi_id);
	if(running_esi->esi_id == esi_id) {
		running_esi->status = S_FINISHED;
		print_and_log_trace(logger, "[FINAL_ESTIMATION][%f]",
				running_esi->estimation);
	}
}

ResourceAllocation * find_allocation_node(int esi_id, int type){
	bool _allocation_esi_node(ResourceAllocation * some_allocation){
		return some_allocation->type == type &&
				some_allocation->esi_id == esi_id;
	}

	return list_find(allocations, (void*) _allocation_esi_node);
}
