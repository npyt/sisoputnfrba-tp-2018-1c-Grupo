#ifndef PLAN_CONSOLE_H_
#define PLAN_CONSOLE_H_

typedef enum{EXIT=0, PAUSE, RESUME,
	BLOCK, UNBLOCK, LIST, KILL,
	STATUS, DEADLOCK, INFO} commands;

void * planner_console_launcher();
int parse(char **, char **, char **);
void execute(int, char*, char*);
int find_in_array(char*, const char**, int);
void string_tolower(char*);

#endif
