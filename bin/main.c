#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include "include/selector.h"
#include "include/main.h"
#include "include/helpers.h"
#include "include/proxyPassiveSocket.h"
#include "include/proxySettings.h"
#include "include/configPassiveSocket.h"

proxy_settings global_settings = {8080, "::", 9090, "127.0.0.1", "", "", 1, 0, 0, 0, 0, 50};

extern char *optarg;
extern int optind, opterr, optopt;
static const int MAXPENDING = 5; // Maximum outstanding connection requests

/* Forwards. */
static int open_server_socket_ipv4(unsigned short port);
static int open_server_socket_ipv6(unsigned short port);
static int open_server_socket_config_ipv4(unsigned short port);
static int open_server_socket_config_ipv6(unsigned short port);
static void parse_arguments(int argc, char *argv[]);


int main(int argc, char *argv[]) {
	parse_arguments(argc, argv);

	int socket_server, socket_server_config;
	selector_status ss      = SELECTOR_SUCCESS;
	fd_selector selector = NULL;

	socket_server = open_server_socket_ipv6(global_settings.proxy_port);
	if (socket_server == -1)
		socket_server = open_server_socket_ipv4(global_settings.proxy_port);
	
	socket_server_config = open_server_socket_config_ipv6(global_settings.management_port);
	if (socket_server_config == -1)
		socket_server_config = open_server_socket_config_ipv4(global_settings.management_port);
	
	const struct selector_init conf = {
		.signal = SIGALRM,
		.select_timeout = {
			.tv_sec  = 10,
			.tv_nsec = 0,
		},
	};

	if(0 != selector_init(&conf)) {
		DieWithSystemMessage("Initializing selector failed");
	}

	selector = selector_new(1024);

	if(selector == NULL) {
		DieWithSystemMessage("Unable to create selector");
	}

	ss = selector_register(selector, socket_server, proxy_passive_socket_fd_handler(),
	                       OP_READ, NULL);
	if(ss != SELECTOR_SUCCESS) {
		DieWithSystemMessage("Registering fd failed");
	}

    ss = selector_register(selector, socket_server_config, config_passive_socket_fd_handler_init(),
                           OP_READ, NULL);
    if(ss != SELECTOR_SUCCESS) {
        DieWithSystemMessage("registering fd failed");
    }

	for(;;) {
		ss = selector_select(selector);
		if(ss != SELECTOR_SUCCESS) {
			DieWithSystemMessage("Runtime error");
		}
	}

	return 0;
}

static void parse_arguments(int argc, char *argv[]) {
	int opt;
	in_port_t port;

	while((opt = getopt(argc, argv, ":e:l:L:M:o:p:t:v")) != -1)  {  
		switch(opt) {
			case 'e':
			//archivo de error
			case 'h':
			//ayuda
			case 'l':
				strncpy(global_settings.proxy_address, optarg, sizeof(global_settings.proxy_address));
				break;
			case 'L':
				strncpy(global_settings.management_address, optarg, sizeof(global_settings.proxy_address));
				break;
			case 'M':
				if (strlen(optarg) < sizeof(global_settings.media_types))
					strncpy(global_settings.media_types, optarg, sizeof(global_settings.media_types) - 1);
				break;
			case 'o':
				port = atoi(optarg);
				if (port == 0)
					printLog("Invalid management port\n");
				else
					global_settings.management_port = port;
				break;
			case 'p':
				port = atoi(optarg);
				if (port == 0)
					printLog("Invalid proxy port\n");
				else
					global_settings.proxy_port = port;
				break;
			case 't':
				if (strlen(optarg) < sizeof(global_settings.transformation_command))
					strncpy(global_settings.transformation_command, optarg, sizeof(global_settings.transformation_command) - 1);
				break;
			case 'v':
				printf("1.0.0\n");
				exit(0);
			break;
		}
    }  
}


static int open_server_socket_ipv4 (unsigned short port) {
	in_port_t servPort = port;

	/* Create socket for incoming connections */
	int servSock;                             // Socket descriptor for server
	if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithSystemMessage("socket() failed");

	/* Construct local address structure */
	struct sockaddr_in servAddr;                                          // Local address
	memset(&servAddr, 0, sizeof(servAddr));                               // Zero out structure
	servAddr.sin_family = AF_INET;                                        // IPv4 address family
	if (inet_pton (AF_INET , global_settings.proxy_address, &servAddr.sin_addr.s_addr) != 1) {
		close(servSock);
		return -1;
	}
	servAddr.sin_port = htons(servPort);                                  // Local port

	setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

	/* Bind to the local address */
	if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
		DieWithSystemMessage("bind() failed");

	/* Mark the socket so it will listen for incoming connections */
	if (listen(servSock, MAXPENDING) < 0)
		DieWithSystemMessage("listen() failed");

	if(selector_fd_set_nio(servSock) == -1) {
		DieWithSystemMessage("setting server flags failed");
	}

	return servSock;
}

static int open_server_socket_ipv6(unsigned short port) {
	in_port_t servPort = port;

	/* Create socket for incoming connections */
	int servSock;                             // Socket descriptor for server
	if ((servSock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithSystemMessage("socket() failed");

	/* Construct local address structure */
	struct sockaddr_in6 servAddr;                                            // Local address
	memset(&servAddr, 0, sizeof(servAddr));                                  // Zero out structure
	servAddr.sin6_family = AF_INET6;                                         // IPv6 address family
	if (inet_pton (AF_INET6, global_settings.proxy_address, &servAddr.sin6_addr) != 1) {
		close(servSock);
		return -1;
	}
	servAddr.sin6_port = htons(servPort);                                    // Proxy port

	setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

	/* Bind to the local address */
	if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
		DieWithSystemMessage("bind() failed");

	/* Mark the socket so it will listen for incoming connections */
	if (listen(servSock, MAXPENDING) < 0)
		DieWithSystemMessage("listen() failed");

	if(selector_fd_set_nio(servSock) == -1) {
		DieWithSystemMessage("Setting server flags failed");
	}

	return servSock;
}

//Creates a SCTP passive socket for config protocol
static int open_server_socket_config_ipv4(unsigned short port){
    in_port_t servPort = port;

    /* Create socket for incoming connections */
    int servSock;                             // Socket descriptor for server
    if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0)
        DieWithSystemMessage("socket() failed");

    /* Construct local address structure */
    struct sockaddr_in servAddr;                                          // Local address
    memset(&servAddr, 0, sizeof(servAddr));                               // Zero out structure
    servAddr.sin_family = AF_INET;                                        // IPv4 address family
    if (inet_pton (AF_INET , global_settings.management_address, &servAddr.sin_addr.s_addr) != 1) {
		close(servSock);
		return -1;
	}
    servAddr.sin_port = htons(servPort);

    setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        DieWithSystemMessage("listen() failed");

    if(selector_fd_set_nio(servSock) == -1)
        DieWithSystemMessage("setting server flags failed");

    return servSock;

}

//Creates a SCTP passive socket for config protocol
static int open_server_socket_config_ipv6(unsigned short port){
    in_port_t servPort = port;

    /* Create socket for incoming connections */
    int servSock;                             // Socket descriptor for server
    if ((servSock = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP)) < 0)
        DieWithSystemMessage("socket() failed");

    /* Construct local address structure */
    struct sockaddr_in6 servAddr;                                              // Local address
    memset(&servAddr, 0, sizeof(servAddr));                                    // Zero out structure
    servAddr.sin6_family = AF_INET6;                                           // IPv4 address family
	if (inet_pton (AF_INET6, global_settings.management_address, &servAddr.sin6_addr) != 1) {
		close(servSock);
		return -1;
	}
    servAddr.sin6_port = htons(servPort);									   // Management port

    setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        DieWithSystemMessage("listen() failed");

    if(selector_fd_set_nio(servSock) == -1)
        DieWithSystemMessage("setting server flags failed");

    return servSock;

}
