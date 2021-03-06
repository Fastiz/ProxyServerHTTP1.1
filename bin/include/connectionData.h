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
    int response_has_finished;
    int status_code;
    int gzip;
    char content_type[100];
    int ssl;
    enum connection_states state;
    char client_name[100];
} connection_data;

#endif //CONNECTIONDATA_H