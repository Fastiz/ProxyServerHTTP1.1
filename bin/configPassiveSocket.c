//
// Created by fastiz on 01/06/19.
//

#include "include/configPassiveSocket.h"

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
#include "include/helpers.h"

#include "include/configActiveSocket.h"


static fd_handler fd = {
        .handle_read = config_passive_socket_read,
        .handle_write = NULL,
        .handle_block = NULL,
        .handle_close = NULL
};

fd_handler * config_passive_socket_fd_handler_init(void){
	return &fd;
}

void config_passive_socket_read(struct selector_key *key){
    struct sockaddr_in clntAddr;        // Client address

    /* Set length of client address structure (in-out parameter) */
    socklen_t clntAddrLen = sizeof(clntAddr);

    /* Wait for a client to connect */
    int clientSocket = accept(key->fd, (struct sockaddr *) &clntAddr, &clntAddrLen);
    if (clientSocket < 0)
        DieWithSystemMessage("accept() failed");

    if(selector_fd_set_nio(clientSocket) == -1) {
        DieWithSystemMessage("setting client flags failed");
    }

    void * clientData = config_active_socket_data_init(key->s, clientSocket);

    /* clientSocket is connected to a client */
    if(SELECTOR_SUCCESS != selector_register(key->s, clientSocket, config_active_socket_fd_handler_init(),
                                             OP_READ, clientData)) {
        DieWithSystemMessage("registering client fd failed");
    }
}
