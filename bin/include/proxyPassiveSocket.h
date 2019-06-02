#ifndef PROXYPASSIVESOCKET_H
#define PROXYPASSIVESOCKET_H

#include "selector.h"

fd_handler * proxy_passive_socket_fd_handler(void);

void proxy_passive_socket_read(struct selector_key *key);

#endif //PROXYPASSIVESOCKET_H
