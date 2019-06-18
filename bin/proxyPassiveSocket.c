#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "include/selector.h"
#include "include/proxyPassiveSocket.h"
#include "include/proxyClientActiveSocket.h"
#include "include/helpers.h"

static fd_handler fd = {
	.handle_read = proxy_passive_socket_read,
	.handle_write = NULL,
	.handle_block = NULL,
	.handle_close = NULL
};

fd_handler * proxy_passive_socket_fd_handler(void){
	return &fd;
}

void proxy_passive_socket_read(struct selector_key *key) {
	struct sockaddr_in clntAddr;        // Client address

	/* Set length of client address structure (in-out parameter) */
	socklen_t clntAddrLen = sizeof(clntAddr);

	/* Wait for a client to connect */
	int clientSocket = accept(key->fd, (struct sockaddr *) &clntAddr, &clntAddrLen);
	if (clientSocket < 0) {
		printf("accept() failed\n");
		return;
	}

	if(selector_fd_set_nio(clientSocket) == -1) {
		close(clientSocket);
		printf("Setting client flags failed\n");
		return;
	}

	void * clientData = proxy_client_active_socket_data_init(key->s, clientSocket);
	if (clientData == NULL) {
		close(clientSocket);
		printf("Client rejected\n");
		return;
	}

	/* clientSocket is connected to a client */
	if(SELECTOR_SUCCESS != selector_register(key->s, clientSocket, proxy_client_active_socket_fd_handler(),
	                                         OP_READ, clientData)) {
		close(clientSocket);
		free(clientData);
		printf("Registering client fd failed\n");
	}

	/*char clntName[INET_ADDRSTRLEN];                                 // String to contain client address
	   if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName,
	              sizeof(clntName)) != NULL)
	        printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
	   else
	        puts("Unable to get client address");*/
}
