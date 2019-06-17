//
// Created by fastiz on 01/06/19.
//

#include "include/configActiveSocket.h"
#include "include/protocolV1.h"

fd_handler config_active_socket_fd_handler_init(void){
    fd_handler fd = {
            .handle_read = read_protocol_v1,
            .handle_write = write_protocol_v1,
            .handle_block = config_active_socket_block,
            .handle_close = close_protocol_v1
    };


}