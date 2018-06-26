#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <chucknorris/allheaders.h>

typedef enum {
	EXIT=0,
	PAUSE,
	RESUME,
	BLOCK,
	UNBLOCK,
	LIST,
	KILL,
	STATUS,
	DEADLOCK,
	INFO
} commands;

//General methods

int is_key_free(char key[KEY_NAME_MAX]);
int is_key_allocated_by(char key[KEY_NAME_MAX], int esi_id);
void process_allocation(ResourceAllocation * ra);
void block_resource_with_esi(char key[KEY_NAME_MAX], int esi_id);
void unlock_resource_with_esi(char key[KEY_NAME_MAX], int esi_id);
void unlock_resource_with_esi_all(char key[KEY_NAME_MAX]);
void free_esis_waiting_for(char key[KEY_NAME_MAX]);
void sort_queues();
int is_esi_waiting(int esi_id);
int get_owner_esi(char key[KEY_NAME_MAX]);
int key_exists(char key[KEY_NAME_MAX]);
void add_esi_to_blocked(ESIRegistration * esi);
ESIRegistration * search_esi(int esi_id);
t_list * search_esis_waiting_for(char key[KEY_NAME_MAX]);
t_list * get_waiting_allocations();
t_list * get_allocations();

//Console

int get_running_flag();
void * planner_console_launcher();
int parse(char **, char **, char **);
void execute(int, char*, char*);
int find_in_array(char*, const char**, int);
void string_tolower(char*);
int deadlock_check(ResourceAllocation*);
int circular_chain(ResourceAllocation* cycle_element, int cycle_head_id);

#endif /* CONSOLE_H_ */
