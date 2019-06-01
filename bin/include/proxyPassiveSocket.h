//
// Created by fastiz on 01/06/19.
//

#ifndef PROXYPASSIVESOCKET_H
#define PROXYPASSIVESOCKET_H

#include "fd_handler.h"

fd_handler proxy_passive_socket_fd_handler_init(void);

void proxy_passive_socket_read(struct selector_key *key);

#endif //PROXYPASSIVESOCKET_H
