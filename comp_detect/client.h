#include <stdint.h>
#define ADDR_LEN 32
#define FIX_DATA_LEN 10

struct configurations {
	char server_ip_addr[ADDR_LEN];
	uint16_t server_port_preprobing;
	uint16_t server_port_postprobing;
	uint16_t udp_src_port;
	uint16_t udp_dst_port;
	unsigned char udp_head_bytes[FIX_DATA_LEN]; // the first 10 bytes after packet id in udp packet payload
	uint32_t l; // the Size of the UDP Payload in a UDP Packet
	uint32_t n; // the Number of Packets in the UDP Packet Train
	uint16_t gamma; // inter-measurement time, γ
};

void pre_probe(char *, struct configurations *);

void probe(struct configurations *);

void post_probe(struct configurations *);
