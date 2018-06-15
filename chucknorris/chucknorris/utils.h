/*
 * utils.h
 *
 *  Created on: 10 jun. 2018
 *      Author: utnso
 */

#ifndef CHUCKNORRIS_UTILS_H_
#define CHUCKNORRIS_UTILS_H_

#include "generalheaders.h"
#include "structures.h";

void print_instruction(InstructionDetail * instruction);
void exit_with_message(char * message, int hola);

void print_and_log_trace(t_log * logger, char * template, ... );
void print_and_log_debug(t_log * logger, char * template, ... );
void print_and_log_info(t_log * logger, char * template, ... );
void print_and_log_error(t_log * logger, char * template, ... );
void print_and_log_warning(t_log * logger, char * template, ... );

#endif /* CHUCKNORRIS_UTILS_H_ */
