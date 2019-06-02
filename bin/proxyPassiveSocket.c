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

static const fd_handler fd = {
	.handle_read = proxy_passive_socket_read,
	.handle_write = NULL,
	.handle_block = NULL,
	.handle_close = NULL
};

fd_handler * proxy_passive_socket_fd_handler(void){
	return &fd;
}

void proxy_passive_socket_read(struct selector_key *key) {
	proxy_client_active_socket_data * data = malloc(sizeof(proxy_client_active_socket_data));
	struct sockaddr_in clntAddr;                                     // Client address
	/* Set length of client address structure (in-out parameter) */
	socklen_t clntAddrLen = sizeof(clntAddr);

	/* Wait for a client to connect */
	int clientSocket = accept(key->fd, (struct sockaddr *) &clntAddr, &clntAddrLen);
	if (clientSocket < 0)
		DieWithSystemMessage("accept() failed");

	if(selector_fd_set_nio(clientSocket) == -1) {
		DieWithSystemMessage("setting server flags failed");
	}

	data->origin_fd = -1;

	/* clientSocket is connected to a client */
	if(SELECTOR_SUCCESS != selector_register(key->s, clientSocket, proxy_client_active_socket_fd_handler(),
	                                         OP_READ, data)) {
		DieWithSystemMessage("registering client fd failed");
	}

	/*char clntName[INET_ADDRSTRLEN];                                 // String to contain client address
	   if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName,
	              sizeof(clntName)) != NULL)
	        printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
	   else
	        puts("Unable to get client address");*/
}
