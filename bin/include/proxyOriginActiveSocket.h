#ifndef PROXYORIGINACTIVESOCKET_H
#define PROXYORIGINACTIVESOCKET_H

#include "selector.h"
#include "buffer.h"

#define MAX_BUFF_SIZE 1024

fd_handler * proxy_origin_active_socket_fd_handler(void);

void * proxy_origin_active_socket_data_init(int client_fd, buffer * read, buffer * write);

void proxy_origin_active_socket_read(struct selector_key *key);

void proxy_origin_active_socket_write(struct selector_key *key);

void proxy_origin_active_socket_block(struct selector_key *key);

void proxy_origin_active_socket_close(struct selector_key *key);

typedef struct {
    int client_fd;
    buffer * read_buff;
    buffer * write_buff;
    int closed;
} proxy_origin_active_socket_data;

#endif //PROXYORIGINACTIVESOCKET_H
