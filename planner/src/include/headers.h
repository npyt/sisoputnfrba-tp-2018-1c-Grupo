#ifndef HEADERS
#define HEADERS

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

#include <string.h>
#include <stdbool.h>

typedef struct {
	int server_socket;
	int coord_socket;
} SocketToListen;

void change_esi_status(ESI *, ESIStatus);
void create_queues();
void define_planner_algorithm(t_config *, PlannerAlgorithm);
void planner_console_launcher();
void * listening_threads(SocketToListen*);

// TESTING CODE

void run_order();
void register_esi(ESI *);
void assign_esi_id(ESI *);
void sort_esi(ESI * , PlannerAlgorithm);

void register_esi_socket(int, char *);
float estimation(int, float);
void sort_queues();
void sort_by_burst();
void * running_thread();


//probndo
void block_esi(char* ESIName);
bool check_Key_availability(char* key_name);
bool check_Key_taken(char* key,char* esi_id);
void load_key(char* key,char* esi_id);
void pre_load_Blocked_keys();
void pload_Keys(char* k_array[],int sizeK);
int array_size(char* array[]);

// END TESTING CODE

#endif
