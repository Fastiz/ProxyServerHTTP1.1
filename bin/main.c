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

static const int MAXPENDING = 5; // Maximum outstanding connection requests

/* Forwards. */
static int open_client_socket( char* hostname, unsigned short port );
static int open_server_socket( unsigned short port );

int main(int argc, char *argv[]) {
	int socketServer;
	selector_status ss      = SELECTOR_SUCCESS;
	fd_selector selector = NULL;

	//ToDo: checkear que el puerto es int
	in_port_t servPort = atoi(argv[1]);         // First arg:  local port
	socketServer = open_server_socket(servPort);

	const struct selector_init conf = {
		.signal = SIGALRM,
		.select_timeout = {
			.tv_sec  = 10,
			.tv_nsec = 0,
		},
	};

	if(0 != selector_init(&conf)) {
		DieWithSystemMessage("initializing selector failed");
	}

	selector = selector_new(1024);

	if(selector == NULL) {
		DieWithSystemMessage("unable to create selector");
	}

	ss = selector_register(selector, socketServer, proxy_passive_socket_fd_handler(),
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

	exit( 0 );
}


static int open_server_socket(unsigned short port) {
	in_port_t servPort = port;

	/* Create socket for incoming connections */
	int servSock;                         // Socket descriptor for server
	if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithSystemMessage("socket() failed");

	/* Construct local address structure */
	struct sockaddr_in servAddr;                                      // Local address
	memset(&servAddr, 0, sizeof(servAddr));                           // Zero out structure
	servAddr.sin_family = AF_INET;                                    // IPv4 address family
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);                     // Any incoming interface
	servAddr.sin_port = htons(servPort);                              // Local port

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
