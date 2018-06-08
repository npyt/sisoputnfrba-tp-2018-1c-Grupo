#ifndef COMMANDS_H_
#define COMMANDS_H_

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
