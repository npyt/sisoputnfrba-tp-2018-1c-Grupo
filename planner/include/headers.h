#ifndef HEADERS
#define HEADERS

#include "structures.h"
#include "sockets.h"
#include "plan_console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

void * listening_thread(int);
void fifo(ESI *);
void sjfcd(ESI *);
void sjfsd(ESI *);
void hrrn(ESI *);
void change_ESI_status(ESI *, ESIStatus);
void create_queues();
void define_algorithm(t_config *, PlannerAlgorithm);
void sort_esi(ESI * , PlannerAlgorithm);



#endif
