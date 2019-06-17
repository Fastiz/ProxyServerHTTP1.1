//
// Created by fastiz on 01/06/19.
//

#include "include/configActiveSocket.h"
#include "include/protocolV1.h"

static fd_handler fd = {
        .handle_read = read_protocol_v1,
        .handle_write = write_protocol_v1,
        .handle_block = config_active_socket_block,
        .handle_close = close_protocol_v1
};


fd_handler * config_active_socket_fd_handler_init(void){
    return &fd;

}

void * config_active_socket_data_init(fd_selector s, int client_fd) {
    return protocol_data_init_v1(s, client_fd);
}

//TODO: esto no va
void config_active_socket_block(struct selector_key *key){
    return;
}