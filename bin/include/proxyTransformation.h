#ifndef PROXYTRANSFORMATION_H
#define PROXYTRANSFORMATION_H

#include "selector.h"
#include "buffer.h"

void * proxy_transformation_data_init(fd_selector s, void * client_data, void * origin_data, int client_fd, int origin_fd);

void proxy_transformation_read(struct selector_key *key);

void proxy_transformation_write(struct selector_key *key);

void proxy_transformation_close(struct selector_key *key);

#endif //PROXYTRANSFORMATION_H
