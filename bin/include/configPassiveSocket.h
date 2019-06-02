//
// Created by fastiz on 01/06/19.
//

#ifndef CONFIGPASSIVESOCKET_H
#define CONFIGPASSIVESOCKET_H

#include "selector.h"

fd_handler config_passive_socket_fd_handler_init(void);

void config_passive_socket_read(struct selector_key *key);

#endif //CONFIGPASSIVESOCKET_H
