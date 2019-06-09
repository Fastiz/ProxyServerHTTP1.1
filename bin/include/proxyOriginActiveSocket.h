#ifndef PROXYORIGINACTIVESOCKET_H
#define PROXYORIGINACTIVESOCKET_H

#include "selector.h"
#include "buffer.h"

#define MAX_BUFF_SIZE 1024

fd_handler * proxy_origin_active_socket_fd_handler(void);

void * proxy_origin_active_socket_data_init(fd_selector s, int client_fd, int origin_fd, void * client_data, buffer * read, buffer * write);

void reset_origin_data(void * origin_data);

void set_origin_transformation_fd(void * origin_data, int fd);

int read_unchunked(void * origin_data, char * dest_buff, int size);

void write_chunked(void * origin_data, char * data_buff, int size);

int available_bytes(void * origin_data);

int response_has_finished(void * origin_data);

void proxy_origin_active_socket_read(struct selector_key *key);

void proxy_origin_active_socket_write(struct selector_key *key);

void proxy_origin_active_socket_block(struct selector_key *key);

void proxy_origin_active_socket_close(struct selector_key *key);

#endif //PROXYORIGINACTIVESOCKET_H
