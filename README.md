# Compression Detect Applications

Developer: Tiansi Gu
This project provides two network applications that detect network compression by endhosts.
- Client/Server Application
- Standalone Application

## Table of contents

- Requirements
- Installation
- Configuration
- Detection
- Design Notes

## Requirements

To build and run the applications, the following environament and modules are required:
- Download [VM Ubuntu ISO](https://drive.google.com/drive/folders/1yZYTqGJdbampVp2MdhifeIlgPGoKruSG) image appropriate to your hardware and follow [VM Instal Instructions](https://docs.google.com/document/d/1ptEYs2jUfzgJ8Ojlpdex4RWdXppA8GJ050ht7pk3z9I/edit?tab=t.0) to set a client VM and a server VM on your machine. 
- Have gcc (version 11.4.0 or above) installed in both the VMs. Inside your VM, run
    ```
    $ sudo apt install gcc
    ```
- Have cjson library installed in both the VMs. You can install it in your VM by running
    ```
    $ sudo apt-get install libjson0 libjson0-dev
    ```


## Installation and Build

- Download comp_detect.zip in both of your VMs. Unzip it:
```
% unzip comp_detect.zip
% cd comp_detect/
```
### Client-Server Application
- In the client VM, run the following command:
```
% make -f Makefile_client
```
- In the server VM, run the following command:
```
% make -f Makefile_server
```

### Standalone Application
- In the client VM, run the following command:
```
% make -f Makefile_standalone
```

## Configuration
The configuration file for the programs is a json file. It has the following fields:
- `server_ip_addr`(String): The Server’s IP Address (You must set)
- `client_ip_addr`(String): The Client’s IP Address (You must set)
- `server_port_preprobing`(Integer): Port Number for TCP (Pre-Probing Phases) (default value: 7777)
- `server_port_postprobing`(Integer): Port Number for TCP (Post-Probing Phases) (default value: 6666)
- `client_port_SYN`(Integer): Source Port Number for TCP SYN (default value: 1234)
- `server_port_head_SYN`(Integer): Destination Port Number for TCP Head SYN
- `server_port_tail_SYN`(Integer): Destination Port Number for TCP Tail SYN
- `udp_src_port`(Integer): Source Port Number for UDP  (default value: 9876)
- `udp_dst_port`(Integer): Destination Port Number for UDP  (default value: 8765)
- `udp_head_bytes`(String): The first 10 Bytes for High Entropy UDP Payload, Should of Length 10 (default value: "1234567890"). This param serves as an identifier between low and high entropy packet as a collective timeout is set for the server to receive two UDP trains. [Related Piazza Discussion](https://piazza.com/class/m62nmzbotec1lq/post/43)
- `l`(Integer): The Size of the UDP Payload in the UDP Packet Train (default value: 1000)
- `n`(Integer): The Number of UDP Packets in one UDP Packet Train (default value: 6000)
- `gamma`(Integer): Inter-Measurement Time (default value: 15)
- `tau`(Integer): The Threshold that we Consider Compression Exist, Don't Change unless Necessary (default value: 100)
- `ttl`(Integer): TTL for the UDP Packets (default value: 255)

Before running the programs, you need to have the public ip address of your client VM and server VM, respectively. Run the following command in your VM, and get ip address from enp0s1 - inet protocol
```
% ip a
```
Put the ip addresses as json strings in field `server_ip_addr` and `client_ip_addr` of `myconfig.json`:
```
{
  "server_ip_addr": "your-client-VM-ip-addr",
  "client_ip_addr": "your-server-VM-ip-addr",
  ......
}
```
Other configuration parameters in `myconfig.json` are preset to default values, and you can change them as needed.
Both of the applications are designed to have default config values defined inside and would work in the absence of config file parameters, except for `server_ip_addr` and `client_ip_addr`. Therefore, in your config json file, you must and should at least have these two ip addresses set up properly. You need to set them in your client VM.

As we send packets aggressively from client, you need to adjust kernel buffer size based on the number of UDP packets in the UDP train (`n` in configurations) and the payload size of each UDP packet (`l` in configurations). Based on the default `n` and `l`, 8MB should be enough for the buffer to fit all the packets in a UDP train.
```
sudo sysctl -w net.core.rmem_max=8388608 //increase the buffer size to be 8MB
```

## Detection
Now, you can run the detection program.

### Client-Server Application
First, start the server
```
% ./compdetect_server 7777
```
Or
```
% ./compdetect_server
```
Note, if you changed `server_port_preprobing` to your custom value, be sure to run the server application with the same port as program parameter
```
% ./compdetect_server [server_port_preprobing]
```
Then start the client for detection
```
% ./compdetect_client myconfig.json
```

Wait for around 1 mins for the detection to complete. Since there is no compression link between the client VM and the server VM. You will see the detection output:
```
% No compression was detected.
```
If you install the server application on a host between which there do have compression link. The output would be:
```
% Compression detected!
```

### Standalone Application
Setup IP Addresses: To run the standalone application, you need to first get the ip address of the server you want to detect and put it in the `server_ip_addr` field of the configuration file. If you want to detect compression between the client and server VM, enter the ip address of the server VM. The `client_ip_addr` is the client VM's IP address.
You will run the standalone application on your client VM:
```
% sudo ./compdetect myconfig.json
```
Wait for around 1 mins for the detection to complete. The output would be the same format as the client/server application:

## Design Notes
### Client-Server Application
- We want to make sure the server is ready to receive udp packets before the client starts sending udp packets. 
The client application waits for a `SERVER_PREP_TIME`, 2 seconds, after pre-probing phase before it starts probing phase. 
- Since UDP does not guarantee delivery and we send packets aggressively, a timeout, `CUTOFF_TIME` is set up in the server. After 60 seconds since the server starts to receive packets, the server will stop waiting and move on to the next phase even though it has not received some of the expected packets. 60 seconds is a reasonably long time for the receiving to get mature and assume the rest of packets are lost.
- We want to ensure the server has completed probing phase and started listening for post-probing phase before the client initiates the post-probing TCP connection with the server. Therefore, we let the client wait for some time between probing and post-probing phase. Since the `CUTOFF_TIME` of the server is 60 seconds, and this timeout starts before the client sends the first UDP packet, 60 seconds would be a reasonable `WAIT_TIME`. 

### Standalone Application
- Multithreading: This application uses multithreading to receive the RST packets (for the head SYN) and send the UDP trains at the same time. 
- Receiver Timeout: The receiver thread listens for the RST packet for the head/tail SYN packets for `CUTOFF_TIME`(60 seconds) until we consider them lost or never generated by the server.  
This timeout is recorded by `t_first_SYN_sent` and `t_curr.tv_sec`. `t_first_SYN_sent` is shared between sender thread (write it) and receiver thread (read it), so a mutex is used to ensure data consistency when it is accessed.
- Wait-Notify: The program would start receiving before it sends any packets. A mutex `lock`, a condition variable `cond`, and a global variable `is_server_ready` are configured, so that if `is_server_ready` is 0 (indicating the receiver is not ready), the sender thread would block (wait) (via `pthread_cond_wait`) until it receives a signal (via `pthread_cond_signal`) from another thread. Once the receiver thread is ready (ready to receive packets), it sets `is_server_ready` to 1.

 