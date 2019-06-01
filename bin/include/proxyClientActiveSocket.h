//
// Created by fastiz on 01/06/19.
//

#ifndef PROXYACTIVESOCKET_H
#define PROXYACTIVESOCKET_H

#include "fd_handler.h"

fd_handler proxy_client_active_socket_fd_handler_init(void);

void proxy_client_active_socket_read(struct selector_key *key);

void proxy_client_active_socket_write(struct selector_key *key);

void proxy_client_active_socket_block(struct selector_key *key);

void proxy_client_active_socket_close(struct selector_key *key);

typedef struct {
    int origin_fd;
} proxy_client_active_socket_data;

#endif //PROXYACTIVESOCKET_H
