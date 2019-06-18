#ifndef PROXYSETTINGS_H
#define PROXYSETTINGS_H

#include <netinet/in.h>

typedef struct {
    in_port_t proxy_port;
    char proxy_address[50];
	in_port_t management_port;
    char management_address[50];
    char media_types[500];
    char transformation_command[500];
    char transformation_error[100];
    int transformation_state;
    int bytes_received;
    int bytes_sent;
    int current_connections;
    int total_connections;
    int max_connections;
} proxy_settings;

#endif //PROXYSETTINGS_H