#ifndef SOCKETS_H_
#define SOCKETS_H_

	#include <sys/socket.h>
	#include <netdb.h>
	#include <errno.h>

	typedef enum {
		// HANDSHAKES
		ESI_PLANNER_HANDSHAKE,
		ESI_PLANNER_HANDSHAKE_OK,
		ESI_COORD_HANDSHAKE,
		ESI_COORD_HANDSHAKE_OK,
		PLANNER_COORD_HANDSHAKE,
		PLANNER_COORD_HANDSHAKE_OK,
		INSTANCE_COORD_HANDSHAKE,
		INSTANCE_COORD_HANDSHAKE_OK,
		// END HANDSHAKES

		INCOMING_INSTANCE,
		INCOMING_ESI,
		UNKNOWN_MSG_TYPE,
		INSTRUCTION_DETAIL_TO_INSTANCE,
		TEST_SEND,
		TEST_RECV,

		// TESTING CODE
		PLANNER_ESI_RUN
		// END TESTING CODE
	} MessageType;

	typedef struct {
		MessageType type;
		int size;
	} MessageHeader;

	int server_start(int port);
	int connect_with_server(char * addr, int port);
	int send_content_with_header(int destination_socket, MessageType type, void *content, int size);
	int send_only_header(int destination_socket, MessageType type);

#endif /* SOCKETS_H_ */
