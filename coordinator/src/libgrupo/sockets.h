#ifndef SOCKETS_H_
#define SOCKETS_H_

	#include <sys/socket.h>
	#include <netdb.h>
	#include <errno.h>

	typedef enum {
		DATA,
		DATA_RECIEVED,
		UNKNOWN_MSG_TYPE
	} MessageTypeConnection;

	typedef enum {
		INCOMING_INSTANCE
	}MessageType;

	typedef struct {
		MessageType type;
		int size;
	} MessageHeader;



	int server_start(int port);
	int connect_with_server(char * addr, int port);
	int send_content_with_header(int destination_socket, MessageType type, void *content, int size);

#endif /* SOCKETS_H_ */
