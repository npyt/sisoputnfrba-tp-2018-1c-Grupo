#ifndef CHUCKNORRIS_SOCKETS_H_
#define CHUCKNORRIS_SOCKETS_H_

#include "generalheaders.h"
#include "structures.h";

#define MAX_SERVER_CLIENTS 1024

int prepare_socket_in_port(int port);
int connect_with_server(char * addr, int port);
void send_header(int destination, MessageHeader * header);
void send_data(int destination, void * data, int data_size);
void send_header_and_data(int destination, MessageHeader * header, void * data, int data_size);
int recieve_header(int origin, MessageHeader * buffer);
int recieve_data(int origin, void * buffer, int data_size);
int recieve_header_and_data(int origin, MessageHeader * buffer_header, void * message_header, int data_size);
void send_message_type(int destination, MessageType type);

#endif /* CHUCKNORRIS_SOCKETS_H_ */
