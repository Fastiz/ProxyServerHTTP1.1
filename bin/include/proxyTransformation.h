#ifndef PROXYTRANSFORMATION_H
#define PROXYTRANSFORMATION_H

#include "selector.h"
#include "buffer.h"
#include "connectionData.h"

void * proxy_transformation_data_init(connection_data * connection_data);

void proxy_transformation_read(struct selector_key *key);

void proxy_transformation_write(struct selector_key *key);

void proxy_transformation_close(struct selector_key *key);

void kill_transformation(connection_data * connection_data);

#endif //PROXYTRANSFORMATION_H
