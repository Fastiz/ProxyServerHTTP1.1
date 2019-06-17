#ifndef PROXYORIGINACTIVESOCKET_H
#define PROXYORIGINACTIVESOCKET_H

#include "selector.h"
#include "buffer.h"
#include "connectionData.h"

#define MAX_BUFF_SIZE 1024

fd_handler * proxy_origin_active_socket_fd_handler(void);

void * proxy_origin_active_socket_data_init(connection_data * connection_data, buffer * read, buffer * write);

int read_unchunked(void * origin_data, char * dest_buff, int size);

void write_chunked(void * origin_data, char * data_buff, int size);

int available_parser_bytes(void * origin_data);

int available_write_bytes(void * origin_data);

void proxy_origin_active_socket_read(struct selector_key *key);

void proxy_origin_active_socket_write(struct selector_key *key);

void proxy_origin_active_socket_block(struct selector_key *key);

void proxy_origin_active_socket_close(struct selector_key *key);

void kill_origin(connection_data * connection_data);

#endif //PROXYORIGINACTIVESOCKET_H
