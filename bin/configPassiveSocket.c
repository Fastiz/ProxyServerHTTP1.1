//
// Created by fastiz on 01/06/19.
//

#include "include/configPassiveSocket.h"

#define NULL 0

fd_handler config_passive_socket_fd_handler_init(void){
    fd_handler fd = {
            .handle_read = config_passive_socket_read,
            .handle_write = NULL,
            .handle_block = NULL,
            .handle_close = NULL
    };
}
