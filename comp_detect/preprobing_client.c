#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "client.h"

/** 
 * This function runs the client task of preprobing phase: creates a TCP socket, connects to the server, 
 * and sends the configuration data (stored in `buffer`) to the server. After sending the data, 
 * the socket is closed. If any errors occur during preprobing, the function prints error message
 * and exits the program.
 * 
 * @param buffer A pointer to the buffer containing the configuration data to be sent to the server.
 * @param configs A pointer to the `configurations` structure that holds the server's IP address and port.
 * 
 * @return void. This function does not return any value but exits the program on failure.
 */
void pre_probe(char* buffer, struct configurations *configs) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
	    perror("Socket creation failed");
	    exit(EXIT_FAILURE);
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));

	in_addr_t server_addr = inet_addr(configs->server_ip_addr);

	sin.sin_family = AF_INET; /* address from Internet, IP address specifically */
	sin.sin_addr.s_addr = server_addr; /* already in network order */
	sin.sin_port = htons(configs->server_port_preprobing); /* convert to network order */

	if (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
		perror("Cannot connect to server");
		close(sock);
		exit(EXIT_FAILURE);
	}
	
	int count = send(sock, buffer, strlen(buffer), 0);

	if (count == -1) {
		perror("Failed to send configurations");
		close(sock);
		exit(EXIT_FAILURE);
	}

	close(sock);
}
