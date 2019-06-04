#ifndef PROXYACTIVESOCKET_H
#define PROXYACTIVESOCKET_H

#include "selector.h"
#include "buffer.h"

fd_handler * proxy_client_active_socket_fd_handler(void);

void proxy_client_active_socket_read(struct selector_key *key);

void proxy_client_active_socket_write(struct selector_key *key);

void proxy_client_active_socket_block(struct selector_key *key);

void proxy_client_active_socket_close(struct selector_key *key);


enum client_states {
	/**
	 *  Reads the first line (explicit mode) or looks
	 *  for the Host header (transparent mode) in the first request
	 */
	READING_HEADER_FIRST_LINE, 

	/**
	 *  Waits for the DNS request to resolve
	 */
	CONNECTING, 
	
	/**
	 *  Transfers bits from the client to the origin server
	 */
	PASSING_CONTENT
};

typedef struct proxy_client_active_socket_data{
    int origin_fd;
    char* hostname;
    unsigned short port;
    buffer * write_buff;
    buffer * read_buff;
    enum client_states state;
	int closed;
} proxy_client_active_socket_data;

#endif //PROXYACTIVESOCKET_H
