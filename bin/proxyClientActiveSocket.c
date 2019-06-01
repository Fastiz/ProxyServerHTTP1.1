//
// Created by fastiz on 01/06/19.
//

#include "include/proxyClientActiveSocket.h"

fd_handler proxy_client_active_socket_fd_handler_init(void){
    fd_handler fd = {
        .handle_read = proxy_client_active_socket_read,
        .handle_write = proxy_client_active_socket_write,
        .handle_block = proxy_client_active_socket_block,
        .handle_close = proxy_client_active_socket_close,
    };
}

void proxy_client_active_socket_read(struct selector_key *key){

}