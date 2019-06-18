#ifndef PROXYSETTINGS_H
#define PROXYSETTINGS_H

#include <netinet/in.h>

typedef struct {
    in_port_t proxy_port;
	in_port_t management_port;
    char media_types[1000];
    char transformation_command[1000];
    int transformation_state;
    int bytes_received;
    int bytes_sent;
    int current_connections;
    int total_connections;
    int max_connections;
} proxy_settings;

#endif //PROXYSETTINGS_H