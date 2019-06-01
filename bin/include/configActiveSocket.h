//
// Created by fastiz on 01/06/19.
//

#ifndef CONFIGACTIVESOCKET_H
#define CONFIGACTIVESOCKET_H

#include "fd_handler.h"

fd_handler config_active_socket_fd_handler_init(void);

void config_active_socket_read(struct selector_key *key);

void config_active_socket_write(struct selector_key *key);

void config_active_socket_block(struct selector_key *key);

void config_active_socket_close(struct selector_key *key);

/*typedef struct {

} config_active_socket_data;*/

#endif //CONFIGACTIVESOCKET_H
