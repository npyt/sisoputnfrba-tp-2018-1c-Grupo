#ifndef HEADERS
#define HEADERS

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

void fifo(ESI *);
void sjfcd(ESI *);
void sjfsd(ESI *);
void hrrn(ESI *);
void change_ESI_status(ESI *, ESIStatus);
void create_queues();
void define_algorithm(t_config *, PlannerAlgorithm);
void sort_esi(ESI * , PlannerAlgorithm);
void planner_console_launcher();

#endif
