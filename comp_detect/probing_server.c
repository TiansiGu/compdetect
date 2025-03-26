#include <stdint.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "server.h"

/** the time that the server would spend to receive UDP packets until we consider the
rest expected packets are lost and move to the next stage */
#define CUTOFF_TIME 60

/** 
 * This function modify the socket descriptor's flags and set it to non-blocking mode.
 * 
 * @param fd The socket descriptor to modify.
 * @return void. The function exits the program if there is an error when getting or setting the flags.
 */
void set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
        perror("Failed to get file status flags by fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }
	int result = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (result == -1) {
        perror("Failed to set O_NONBLOCK flag");
		close(fd);
        exit(EXIT_FAILURE);
    }
}

/** 
 * This function attempts to increase the buffer allocated by the operating system to 
 * store incoming data before it is read by the application.
 * 
 * @param fd file descriptor of the socket whose receive buffer size is to be increased.
 * 
 * @return void. Exits the program on failure.
 */
void increase_sys_rcvbuf_size(int fd) {
	int rcvbuf_size = 8 * 1024 * 1024;  // 8MB
	int res = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, sizeof(rcvbuf_size));
	if (res == -1) {
		perror("Failed to increase system rcv buffersize");
		close(fd);
        exit(EXIT_FAILURE);
	}
}

/**
 * This function checks if a given buffer corresponds to low or high entropy data.
 * corresponds to low or high entropy data. It checks whether the buffer matches either 
 * of the data heads up to the specified length (`head_len`).

 * @param buf A pointer to the buffer containing the data to check.
 * @param high_entropy_data_head A pointer to the predefined low entropy data head for comparison.
 * @param high_entropy_data_head A pointer to the predefined high entropy data head for comparison.
 * @param head_len The length of the data head to compare.
 * 
 * @return 
 * - 0 if the buffer matches the low entropy data head.
 * - 1 if the buffer matches the high entropy data head.
 * - -1 if the buffer does not match either data head.
 */
int check_entropy(unsigned char *buf, unsigned char *low_entropy_data_head, 
	unsigned char *high_entropy_data_head, int head_len) {

	if (memcmp(buf, low_entropy_data_head, head_len) == 0) {
		return 0;
	} else if (memcmp(buf, high_entropy_data_head, head_len) == 0) {
		return 1;
	} else {
		return -1;
	}
}

/** 
 * This function listens for incoming UDP packets on a socket, differentiate low or high, 
 * entropy packets, and tracks the arrival time of the first and last received packets of 
 * each packet train. The receiving will stop after receiving the required number of packets, or a 
 * collective timeout (CUTOFF_TIME) is reached.
 * Once done with receiving, the function calculates the difference the difference in arrival
 * time between the first and last received packets of the two trains.
 * 
 * @param sock The socket file descriptor for receiving the packets.
 * @param cin A pointer to the client address structure to store the sender's address.
 * @param cin_len Length of the client's address.
 * @param configs A pointer to the `configurations` structure.
 * 
 * @return The time difference the difference in arrival time between the first and last packets of the two trains.
 */
long receive_packet_trains(int sock, struct sockaddr *cin, socklen_t cin_len, struct configurations *configs) {
	int buf_len = configs->l;
	unsigned char buf[buf_len];
	int n = configs->n;
	int count;
	struct timespec t_l1, t_ln, t_h1, t_hn;
	struct timespec t_init, t_curr;
	clock_gettime(CLOCK_MONOTONIC, &t_init);
	
	int low_count = 0, high_count = 0;
	unsigned char low_entropy_data_head[FIX_DATA_LEN] = {0};
	while (1) {
		count = recvfrom(sock, buf, buf_len, 0, cin, &cin_len);
		if (count == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				clock_gettime(CLOCK_MONOTONIC, &t_curr);
				if (t_curr.tv_sec - t_init.tv_sec > CUTOFF_TIME) {
					break;
				} else {
					continue; // No data available (non-blocking)
				}
			} else {
				perror("Failed to receive UDP packet");
				close(sock);
				exit(EXIT_FAILURE);
			}
		}
		
		int entropy = check_entropy(buf + sizeof(uint16_t), low_entropy_data_head, configs->udp_head_bytes, 10);
		if (entropy == 0) {
			if (low_count == 0) clock_gettime(CLOCK_MONOTONIC, &t_l1);
			clock_gettime(CLOCK_MONOTONIC, &t_ln);
			low_count++;
		} else if (entropy == 1) {
			if (high_count == 0) clock_gettime(CLOCK_MONOTONIC, &t_h1);
			clock_gettime(CLOCK_MONOTONIC, &t_hn);
			high_count++;
		} 

		if (low_count == n && high_count == n) {
			break;
		}
	}
    
	// arrival time between first and last packet for low entropy packet train
	long t_l = (t_ln.tv_sec - t_l1.tv_sec) * 1000L + (t_ln.tv_nsec - t_l1.tv_nsec) / 1000000L;
	// arrival time between first and last packet for long entropy packet train
	long t_h = (t_hn.tv_sec - t_h1.tv_sec) * 1000L + (t_hn.tv_nsec - t_h1.tv_nsec) / 1000000L;
	return t_h - t_l;
}

/** 
 * This function performs server's probing task: Receives UDP packet trains, calculates the time difference, 
 * and set `detect_result`based on the calculated time difference and threshold `tau`.
 * 
 * @param configs A pointer to the `configurations` structure.
 * @param detect_result A pointer to an integer that will hold the detection result
 *                      (1 for detected, 0 for not detected).
 * 
 * @return void. This function makes the detection decision and modifies the `detect_result` based on the time difference.
 */
void serve_probe(struct configurations *configs, int *detect_result) {
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
	    perror("Socket creation failed");
	    exit(EXIT_FAILURE);
	}

	// Assign address to socket
	struct sockaddr_in sin, cin;
	memset(&sin, 0, sizeof(sin));

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(configs->udp_dst_port);

	if (bind(sock, (struct sockaddr*) &sin, sizeof(sin)) == -1) {
		perror("Cannot bind socket to address");
		close(sock);
		exit(EXIT_FAILURE);
	}

	// Set non-blocking
	set_nonblocking(sock);

	// Increase sys buf size
	increase_sys_rcvbuf_size(sock);

	// Receive diagrams and caculate time difference
	socklen_t cin_len = sizeof(cin);
	long time_difference = receive_packet_trains(sock, (struct sockaddr *)&cin, cin_len, configs);

	if (time_difference > configs->tau) {
		*detect_result = 1;
	} else {
		*detect_result = 0;
	}

	close(sock);
}
