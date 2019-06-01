//
// Created by fastiz on 01/06/19.
//

#ifndef PROXYORIGINACTIVESOCKET_H
#define PROXYORIGINACTIVESOCKET_H

#include "fd_handler.h"

fd_handler proxy_origin_active_socket_fd_handler_init(void);

void proxy_origin_active_socket_read(struct selector_key *key);

void proxy_origin_active_socket_write(struct selector_key *key);

void proxy_origin_active_socket_block(struct selector_key *key);

void proxy_origin_active_socket_close(struct selector_key *key);

/*typedef struct {

} proxy_origin_active_socket_data;*/

#endif //PROXYORIGINACTIVESOCKET_H
