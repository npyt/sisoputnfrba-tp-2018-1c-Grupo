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
		INSTRUCTION_DETAIL_TO_COODRINATOR,
		TEST_SEND,
		TEST_RECV,

		INSTRUCTION_DETAIL_TO_COORDINATOR,

		CAN_ESI_GET_KEY,
		CAN_ESI_SET_KEY,
		CAN_ESI_STORE_KEY,
		PLANNER_COORDINATOR_OP_OK,
		PLANNER_COORDINATOR_OP_FAILED,

		// TESTING CODE
		PLANNER_ESI_RUN,
		OPERATION_ERROR,
		ESI_EXECUTION_LINE_OK,
		ESI_EXECUTION_FINISHED,
		COORD_ESI_EXECUTED
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
