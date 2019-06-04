#ifndef PROXYACTIVESOCKET_H
#define PROXYACTIVESOCKET_H

#include "selector.h"
#include "buffer.h"

fd_handler * proxy_client_active_socket_fd_handler(void);

void * proxy_client_active_socket_data_init();

void proxy_client_active_socket_read(struct selector_key *key);

void proxy_client_active_socket_write(struct selector_key *key);

void proxy_client_active_socket_block(struct selector_key *key);

void proxy_client_active_socket_close(struct selector_key *key);

#endif //PROXYACTIVESOCKET_H
