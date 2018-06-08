#ifndef COMMANDS_H_
#define COMMANDS_H_

#include <commons/config.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

t_list * taken_keys;
t_list * ready_queue;
t_list * blocked_queue;
t_list * finished_queue;
t_list * running_queue;
t_list * esi_sockets_list;

void pause();
void resume();
void block(char*, char*);
void unblock(char*);
void list(char*);
void kill(char*);
void status(char*);
void deadlock();
void info();

#endif /* COMMANDS_H_ */
