//
// Created by fastiz on 01/06/19.
//

#include "include/configActiveSocket.h"

fd_handler config_active_socket_fd_handler_init(void){
    fd_handler fd = {
            .handle_read = config_active_socket_read,
            .handle_write = config_active_socket_write,
            .handle_block = config_active_socket_block,
            .handle_close = config_active_socket_close
    };
}