//
// Created by fastiz on 01/06/19.
//

#ifndef PROXYACTIVESOCKET_H
#define PROXYACTIVESOCKET_H

void proxy_client_active_socket_read(struct selector_key *key);

void proxy_client_active_socket_write(struct selector_key *key);

void proxy_client_active_socket_block(struct selector_key *key);

void proxy_client_active_socket_close(struct selector_key *key);

/*typedef struct {

} proxy_client_active_socket_data;*/


#endif //PROXYACTIVESOCKET_H
