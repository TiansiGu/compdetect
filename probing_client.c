#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "client.h"
#include "payload_generator.h"

/**
 * This function binds the provided socket descriptor to the specified port.
 * 
 * @param fd The socket descriptor that will be bound to the specified port.
 * @param port The port number to bind the socket to.
 * @param addr The sockaddr_in structure that will hold the bound address and port.
 */
void bind_port(int fd, int port, struct sockaddr_in *addr) {
	addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(port);

	if (bind(fd, (struct sockaddr*) addr, sizeof(struct sockaddr_in)) == -1) {
		perror("Failed to bind socket");
		close(fd);
		exit(EXIT_FAILURE);
	}
}

/** 
 * Sets the "Don't Fragment" (DF) flag on the socket to prevent packet fragmentation.
 * @param fd The file descriptor of the socket to configure.
 */
void set_df(int fd) {
	int val = IP_PMTUDISC_DO;
	if (setsockopt(fd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val)) == -1) {
		perror("Failed to set don't fragment");
		close(fd);
		exit(EXIT_FAILURE);
	}
}

/** 
 * This function runs the client task of probing phase, creates a UDP socket, binds it to a specified source port, 
 * and sends two series of UDP packets to a server. The payload of packets are generated using 
 * the `generate_payload` function, their first 10 bytes are determined by the configuration
 * `udp_head_bytes`, and their IDs are set. After sending the low entropy packet train, the function 
 * waits for a specified time (`gamma`) before sending the high entropy train.
 * 
 * @param configs A pointer to the `configurations` structure containing config params
 * @return void. This function does not return any value but exits on failure.
 */
void probe(struct configurations *configs) {
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
	    perror("Socket creation failed");
	    exit(EXIT_FAILURE);
	}

	struct sockaddr_in client_sin, server_sin;
	memset(&client_sin, 0, sizeof(client_sin));
	memset(&server_sin, 0, sizeof(server_sin));

	in_addr_t server_addr = inet_addr(configs->server_ip_addr);

    server_sin.sin_family = AF_INET; /* address from Internet, IP address specifically */
	server_sin.sin_addr.s_addr = server_addr; /* already in network order */
	server_sin.sin_port = htons(configs->udp_dst_port); /* convert to network order */

	// Specify the socket client uses to connect to server
	bind_port(sock, configs->udp_src_port, &client_sin);

	// Set DF bit
	set_df(sock);

	unsigned char *low_entropy_payload = generate_payload(configs->l, 0);
	unsigned char *high_entropy_payload = generate_payload(configs->l, 1);
	// Set the first 10 bytes to data regulated by configs
	strncpy(high_entropy_payload + sizeof(uint16_t), configs->udp_head_bytes, FIX_DATA_LEN);
	
	int count;
	// Send low entropy packet train
	for (int i = 0; i < configs->n; i++) {
		fill_packet_id(low_entropy_payload, i);
		count = sendto(sock, low_entropy_payload, configs->l, 0, (struct sockaddr *) &server_sin, sizeof(server_sin));
		if (count == -1) {
			perror("Failed to send UDP packets with low entropy data");
			free(low_entropy_payload);
			close(sock);
			exit(EXIT_FAILURE);
		}
	}
	free(low_entropy_payload); //free allocated resources

	// Wait γ secs before sending the high entropy packet train
	sleep(configs->gamma);
	
	// Send high entropy packet train
	for (int i = 0; i < configs->n; i++) {
		fill_packet_id(high_entropy_payload, i);
		count = sendto(sock, high_entropy_payload, configs->l, 0, (struct sockaddr *) &server_sin, sizeof(server_sin));
		if (count == -1) {
			perror("Failed to send UDP packets with high entropy data");
			free(high_entropy_payload);
			close(sock);
			exit(EXIT_FAILURE);
		}
	}
	free(high_entropy_payload);
	
	close(sock);
}
