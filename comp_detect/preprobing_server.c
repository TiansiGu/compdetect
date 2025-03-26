#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "server.h"

/** 
 * This function runs the server task of preprobing: sets up a server to listen for incoming TCP
 * connections on the specified port (`preprobing_port`). Once a connection is accepted, it receives 
 * config information from the client and stores it in the input buffer.
 * After receiving the data, it closes the client and server sockets.
 * 
 * @param preprobing_port The port on which the server will listen for incoming connections.
 * @param buffer A pointer to the buffer where the received data will be stored.
 * @param buffer_len The size of the buffer that will hold the received data.
 * 
 * @return void. This function does not return any value but exits on failure.
 */
void serve_pre_probe(uint16_t preprobing_port, char *buffer, int buffer_len) {
	// Create socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
	    perror("Socket creation failed");
	    exit(EXIT_FAILURE);
	}

	// Assign address to socket
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(preprobing_port);

	if (bind(sock, (struct sockaddr*) &sin, sizeof(sin)) == -1) {
		perror("Cannot bind socket to address");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// Start listening 
	if (listen(sock, 5) == -1) {
		perror("Cannot listen incoming connection requests");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// Accept incoming connection - in this phase just need one connection
	int client_sock = accept(sock, NULL, NULL); //don't need client's IP addr
	if (client_sock == -1) {
		perror("Failed to accept connection");
		close(sock);
		exit(EXIT_FAILURE);
	}

	int count = recv(client_sock, buffer, buffer_len, 0);
	if (count == -1) {
		perror("Failed to receive");
		close(client_sock);
		close(sock);
		exit(EXIT_FAILURE);
	}
	buffer[count] = '\0';
	//printf("The server received %s\n", buffer); //to be deleted

	close(client_sock);
	close(sock);
}
