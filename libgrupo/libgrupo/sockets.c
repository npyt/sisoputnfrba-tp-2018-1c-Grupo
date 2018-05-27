#include "sockets.h"

int server_start(int port) {
	int server_socket;
	struct sockaddr_in server_addr;

	server_socket = socket(PF_INET, SOCK_STREAM, 0);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

	int activado = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

	if(bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
		return -1;
	}
	return server_socket; //Retorna el socket con el puerto bindeado
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

int send_content_with_header(int destination_socket, MessageType type, void *content, int size) {
	MessageHeader * header = malloc(sizeof(MessageHeader));
	(*header).type = type;
	(*header).size = size;

	send(destination_socket, header, sizeof(MessageHeader), 0);

	if(size > 0) {
		send(destination_socket, content, size, 0);
	}
	return 0;
}

int send_only_header(int destination_socket, MessageType type) {
	MessageHeader * header = malloc(sizeof(MessageHeader));
	(*header).type = type;
	(*header).size = 0;

	send(destination_socket, header, sizeof(MessageHeader), 0);
	return 0;
}
