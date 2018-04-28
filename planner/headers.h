#ifndef HEADERS
#define HEADERS

#include "structures.h"
#include "sockets.h"
#include "plan_console.h"

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

void * listening_thread(int);
void fifo(ESI *, ESIStatus *);
void change_ESI_status(ESI*, ESIStatus * );


#endif
