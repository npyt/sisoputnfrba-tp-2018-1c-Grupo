#ifndef HEADERS
#define HEADERS

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

#include <string.h>

void change_ESI_status(ESI *, ESIStatus);
void create_queues();
void define_planner_algorithm(t_config *, PlannerAlgorithm);
void planner_console_launcher();
void * listening_thread(int);

// TESTING CODE

void register_esi(ESI *);
void assign_esi_id(ESI *);
void sort_esi(ESI * , PlannerAlgorithm);

// END TESTING CODE

#endif
