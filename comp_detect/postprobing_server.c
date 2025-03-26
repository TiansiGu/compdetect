#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "server.h"

#define COMPRESSION_MSG "Compression detected!"
#define NO_COMPRESSION_MSG "No compression was detected."

/** 
 * This function performs server's post-probing task: creates a TCP socket, listens for 
 * incoming connections, and sends a detection result message based on the `detect` value.
 * If compression is detected (`detect`is 1), it sends `COMPRESSION_MSG`; Otherwise, 
 * it sends `NO_COMPRESSION_MSG` to the client.
 * 
 * @param postprobing_port The port to listen for incoming connections.
 * @param detect The detection result (1 for compression, 0 for no compression) decided in probing phase.
 * 
 * @return void. Exits on failure during socket creation, binding, accepting, or data sending.
 */
void serve_post_probe(uint16_t postprobing_port, int detect) {
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
	sin.sin_port = htons(postprobing_port);

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

	int count;
	if (detect) {
		send(client_sock, COMPRESSION_MSG, strlen(COMPRESSION_MSG), 0);
	} else {
		send(client_sock, NO_COMPRESSION_MSG, strlen(NO_COMPRESSION_MSG), 0);
	}
	if (count == -1) {
		perror("Failed to send detection results to client");
		close(client_sock);
		close(sock);
		exit(EXIT_FAILURE);
	}
	
	close(client_sock);
	close(sock);
}
