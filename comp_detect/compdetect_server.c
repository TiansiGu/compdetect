#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include "server.h"
#include "default.h"

#define BUFFER_SIZE 1024

/** 
 * This function parse the server's preprobing port number from command line argument.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return The preprobing port of server.
 */
uint16_t parse_preprobing_port(int argc, char* argv[]) {
	if (argc < 2) return 7777;
	return atoi(argv[1]);
}

/** 
 * This function reads a JSON configuration file, parses its contents, extracts 
 * configuration values, and stores them in the provided `configs` structure. If 
 * a specific field doesn't exist in the json file, set the field of `configs`
 * to default value (defined in default.h).
 * 
 * @param file_name The name of the configuration file to be parsed.
 * @param buffer A buffer where the contents of the configuration file will be stored temporarily.
 * @param configs A pointer to the `configs` structure.
 * 
 * @return void. This function modifies the `configs` structure based on the parsed configuration data.
 */
void parse_configs(char *buffer, struct configurations *configs) {
	// parse the JSON data 
	cJSON *json = cJSON_Parse(buffer); 
	if (json == NULL) { 
		const char *error_ptr = cJSON_GetErrorPtr(); 
		if (error_ptr != NULL) { 
	    	 printf("Error when parsing json str: %s\n", error_ptr); 
	    } 
	    cJSON_Delete(json); 
	    exit(1); 
	}
	
	// access the JSON data 
	cJSON *name = cJSON_GetObjectItemCaseSensitive(json,"server_port_postprobing"); 
	if (cJSON_IsNumber(name)) { 
		configs->server_port_postprobing = name->valueint;
	} else {
		configs->server_port_postprobing = DEFAULT_SERVER_PORT_POSTPROBING;
	}

	name = cJSON_GetObjectItemCaseSensitive(json,"udp_dst_port"); 
	if (cJSON_IsNumber(name)) { 
		configs->udp_dst_port = name->valueint;
	} else {
		configs->udp_dst_port = DEFAULT_UDP_DST_PORT;
	}
	
	name = cJSON_GetObjectItemCaseSensitive(json,"udp_head_bytes"); 
	if (cJSON_IsString(name) && (name->valuestring != NULL)) { 
		memcpy(configs->udp_head_bytes, name->valuestring, FIX_DATA_LEN);
	} else {
		memcpy(configs->udp_head_bytes, DEFAULT_UDP_HEAD_BYTES, FIX_DATA_LEN);
	}

	name = cJSON_GetObjectItemCaseSensitive(json,"l");
	if (cJSON_IsNumber(name)) {
		configs->l = name->valueint;
	} else {
		configs->l = DEFAULT_L;
	}

	name = cJSON_GetObjectItemCaseSensitive(json,"n");
	if (cJSON_IsNumber(name)) {
		configs->n = name->valueint;
	} else {
		configs->n = DEFAULT_N;
	}
	
	name = cJSON_GetObjectItemCaseSensitive(json,"tau");
	if (cJSON_IsNumber(name)) {
		configs->tau = name->valueint;
	} else {
		configs->tau = DEFAULT_TAU;
	}
	  
	// delete the JSON object 
	cJSON_Delete(json);  
}

/** 
 * This function checks if the configuration file is provided, then parses the configuration 
 * file, and execute three detection processes: preprocessing, probing, and postprobing.
 * 
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments.
 * @return EXIT_SUCCESS on successful completion, or EXIT_FAILURE if an error occurs.
 */
int main(int argc, char* argv[]) {
	struct configurations configs;
	uint16_t preprobing_port = parse_preprobing_port(argc, argv);
	char buffer[BUFFER_SIZE];
	serve_pre_probe(preprobing_port, buffer, BUFFER_SIZE - 1);
	parse_configs(buffer, &configs);
	
	int detect_result = 0;
	serve_probe(&configs, &detect_result);

	serve_post_probe(configs.server_port_postprobing, detect_result);

	return 0;
}

