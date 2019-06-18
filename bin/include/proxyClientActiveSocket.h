#ifndef PROXYACTIVESOCKET_H
#define PROXYACTIVESOCKET_H

#include "selector.h"
#include "connectionData.h"

fd_handler * proxy_client_active_socket_fd_handler(void);

void * proxy_client_active_socket_data_init(fd_selector s, int client_fd);

void proxy_client_active_socket_read(struct selector_key *key);

void proxy_client_active_socket_write(struct selector_key *key);

void proxy_client_active_socket_block(struct selector_key *key);

void proxy_client_active_socket_close(struct selector_key *key);

void set_client_name(void * client_data, char * client_name);

void kill_client(connection_data * connection_data);

#endif //PROXYACTIVESOCKET_H
