//
// Created by fastiz on 01/06/19.
//

#include "include/proxyOriginActiveSocket.h"

fd_handler proxy_origin_active_socket_fd_handler_init(void){
    fd_handler fd = {
            .handle_read = proxy_origin_active_socket_read,
            .handle_write = proxy_origin_active_socket_write,
            .handle_block = proxy_origin_active_socket_block,
            .handle_close = proxy_origin_active_socket_close,
    };
}