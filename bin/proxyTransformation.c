#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include "include/helpers.h"
#include "include/proxyTransformation.h"
#include "include/proxyOriginActiveSocket.h"
#include "include/proxyClientActiveSocket.h"

#define READ 0
#define WRITE 1

static fd_handler fd = {
	.handle_read = proxy_transformation_read,
	.handle_write = proxy_transformation_write,
	.handle_block = NULL,
	.handle_close = proxy_transformation_close
};

fd_handler * proxy_transformation_handler(void){
	return &fd;
}

typedef struct proxy_transformation_data {
	int origin_pipe[2];
	int client_pipe[2];
	int client_fd;
	int origin_fd;
	void * origin_data;
} proxy_transformation_data;

void * proxy_transformation_data_init(fd_selector s, void * client_data, void * origin_data, int client_fd, int origin_fd) {
	proxy_transformation_data * data = malloc(sizeof(proxy_transformation_data));
	data->origin_data = origin_data;
	data->client_fd = client_fd;
	data->origin_fd = origin_fd;
	pipe(data->client_pipe);
	pipe(data->origin_pipe);
	set_client_transformation_fd(client_data, data->client_pipe[READ]);
	set_origin_transformation_fd(origin_data, data->origin_pipe[WRITE]);

	char * args[] = {"sed", "s/ACME/ITBA/g", NULL};
	pid_t pid = fork();
	if (pid < 0) {
		send_error(500, "Internal server error", (char*) 0, "Coudn't execute transformation");
	}

	if (pid == 0) {
		dup2(data->origin_pipe[READ], 0);
		dup2(data->client_pipe[WRITE], 1);
		close(data->origin_pipe[WRITE]);
		close(data->client_pipe[READ]);
		execvp(args[0], args);
	}

	close(data->client_pipe[WRITE]);
	close(data->origin_pipe[READ]);

	if(selector_fd_set_nio(data->client_pipe[READ]) == -1) {
		DieWithSystemMessage("setting transformer flags failed");
	}

	if(selector_fd_set_nio(data->origin_pipe[WRITE]) == -1) {
		DieWithSystemMessage("setting transformer flags failed");
	}

	if(SELECTOR_SUCCESS != selector_register(s, data->client_pipe[READ], proxy_transformation_handler(),
	                                         OP_READ, data))
		DieWithSystemMessage("registering fd failed");
	if(SELECTOR_SUCCESS != selector_register(s, data->origin_pipe[WRITE], proxy_transformation_handler(),
	                                         OP_WRITE, data))
		DieWithSystemMessage("registering fd failed");

	return data;
}

void proxy_transformation_read(struct selector_key *key) {
	proxy_transformation_data * data = key->data;

	char aux[1000];
	int ret = -1;
	int bytes_to_send;

	do {
		bytes_to_send = sizeof(aux) > available_response_bytes(data->origin_data) ? available_response_bytes(data->origin_data) : sizeof(aux);
		if (bytes_to_send == 0) {
			/* Buffer full. Must wait for the client to read it */
			selector_set_interest_key(key, OP_NOOP);
			break;
		}
		if ((ret = read(data->client_pipe[READ], aux, bytes_to_send)) <= 0)
			break;
		write_chunked(data->origin_data, aux, ret);
	} while (ret > 0);

	if (ret == 0) {
		/* Connection was closed. End of response. */
		write_chunked(data->origin_data, "", 0);
		selector_unregister_fd(key->s, key->fd);
	}

	selector_set_interest(key->s, data->client_fd, OP_WRITE);
}

void proxy_transformation_write(struct selector_key *key) {
	proxy_transformation_data * data = key->data;
	char aux[1000];
	int ret;

	while ((ret = read_unchunked(data->origin_data, aux, sizeof(aux))) > 0) {
		write(data->origin_pipe[WRITE], aux, ret);
	}

	if (response_has_finished(data->origin_data) == 1) {
		selector_unregister_fd(key->s, key->fd);
	}

	selector_set_interest_key(key, OP_NOOP);
	selector_set_interest(key->s, data->origin_fd, OP_READ);
}

void proxy_transformation_close(struct selector_key *key) {
	close(key->fd);
}
