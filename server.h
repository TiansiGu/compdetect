#include <stdint.h>
#define ADDR_LEN 32
#define FIX_DATA_LEN 10

struct configurations {
	uint16_t server_port_postprobing;
	uint16_t udp_dst_port;
	unsigned char udp_head_bytes[FIX_DATA_LEN];  // the first 10 bytes after packet id in udp packet payload
	uint32_t l; // the Size of the UDP Payload in a UDP Packet
	uint32_t n; // the Number of Packets in the UDP Packet Train
	uint16_t tau; // threshold of time diff (in millis) between low and high entropy data
};

void serve_pre_probe(uint16_t, char *, int);

void serve_probe(struct configurations *, int *);

void serve_post_probe(uint16_t, int);
