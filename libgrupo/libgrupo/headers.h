#ifndef SRC_HEADERS_H_
#define SRC_HEADERS_H_

#include "structures.h"
#include "sockets.h"

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>

void * listening_thread(int server_socket);
void * coordinator_listening_thread(void);
void * planner_listening_thread(void);

#endif /* SRC_HEADERS_H_ */
