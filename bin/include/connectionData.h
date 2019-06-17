#ifndef CONNECTIONDATA_H
#define CONNECTIONDATA_H

#include "selector.h"

enum connection_states {
    ALIVE,
    CLOSED,
    TRANSFORMER_MUST_CLOSE
};

typedef struct {
    fd_selector s;
	int client_fd;
    void * client_data;
    int origin_fd;
    void * origin_data;
	int client_transformation_fd;
    int origin_transformation_fd;
	void * transformation_data;
    char content_type[100];
    int ssl;
    enum connection_states state;  
} connection_data;

#endif //CONNECTIONDATA_H