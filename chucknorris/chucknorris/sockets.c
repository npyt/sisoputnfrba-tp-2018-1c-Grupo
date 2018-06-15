#include "sockets.h"

int prepare_socket_in_port(int port) {
	int server_socket;
	struct sockaddr_in server_addr;

	server_socket = socket(PF_INET, SOCK_STREAM, 0);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

	int active = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &active, sizeof(active));

	if(bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
		return -1;
	}
	return server_socket;
}

int connect_with_server(char * addr, int port) {
	int client_socket;
	char buffer[1024];
	struct sockaddr_in serverAddr;
	socklen_t addr_size;

	client_socket = socket(PF_INET, SOCK_STREAM, 0);

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(addr);
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

	addr_size = sizeof serverAddr;
	if (connect(client_socket, (struct sockaddr *) &serverAddr, addr_size) == -1) {
		return -1;
	}
	return client_socket;
}

void send_header(int destination, MessageHeader * header) {
	send(destination, header, sizeof(MessageHeader), 0);
}

void send_data(int destination, void * data, int data_size) {
	send(destination, data, data_size, 0);
}

void send_header_and_data(int destination, MessageHeader * header, void * data, int data_size) {
	send_header(destination, header);
	send_data(destination, data, data_size);
}

int recieve_header(int origin, MessageHeader * buffer) {
	return recv(origin, buffer, sizeof(MessageHeader), 0);
}

int recieve_data(int origin, void * buffer, int data_size) {
	return recv(origin, buffer, data_size, 0);
}

int recieve_header_and_data(int origin, MessageHeader * buffer_header, void * message_header, int data_size) {
	return recieve_header(origin, buffer_header) + recieve_data(origin, message_header, data_size);
}


void send_message_type(int destination, MessageType type) {
	MessageHeader * header = malloc(sizeof(MessageHeader));
	header->type = type;
	send_header(destination, header);
}
