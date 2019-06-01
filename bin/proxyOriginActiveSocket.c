//
// Created by fastiz on 01/06/19.
//

#include <sys/socket.h>
#include "include/proxyOriginActiveSocket.h"
#include "include/helpers.h"
#include <errno.h>

fd_handler proxy_origin_active_socket_fd_handler_init(void){
    fd_handler fd = {
            .handle_read = proxy_origin_active_socket_read,
            .handle_write = proxy_origin_active_socket_write,
            .handle_block = proxy_origin_active_socket_block,
            .handle_close = proxy_origin_active_socket_close,
    };
}

void proxy_origin_active_socket_write(struct selector_key *key){
    char buff[MAX_BUFF_SIZE];
    int bytesRead = recv(key->data->client_fd, buff, MAX_BUFF_SIZE, 0);
    if(bytesRead == -1)
        DieWithSystemMessage("Error when reading client socket.");

    int result = send(key->fd, buff, bytesRead, 0);
    if(result == -1){
        if(errno == EWOULDBLOCK){
            //key->data->
        }else{
            DieWithSystemMessage("Error when writing origin socket.")
        }
    }
}