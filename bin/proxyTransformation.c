#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "include/helpers.h"
#include "include/proxyTransformation.h"
#include "include/proxyOriginActiveSocket.h"
#include "include/proxyClientActiveSocket.h"
#include "include/proxySettings.h"

#define READ 0
#define WRITE 1

extern proxy_settings global_settings;

int media_type_match(char * media_types, char * content_type);

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
	buffer * tmp_buff;
	connection_data * connection_data;
} proxy_transformation_data;

void * proxy_transformation_data_init(connection_data * connection_data) {
	if (connection_data->status_code < 200 || connection_data->status_code >= 300)
		return NULL;

	if (global_settings.transformation_state == 0 || strcmp(global_settings.transformation_command, "") == 0)
		return NULL;

	if  (media_type_match(global_settings.media_types, connection_data->content_type) == 0)
		return NULL;

	proxy_transformation_data * data = malloc(sizeof(proxy_transformation_data));
	if (data == NULL)
		return NULL;

	data->connection_data = connection_data;
	connection_data->transformation_data = data;
	if (pipe(data->client_pipe) < 0) {
		return NULL;
	}
	if (pipe(data->origin_pipe) < 0) {
		close(data->client_pipe[WRITE]);
		close(data->client_pipe[READ]);
		return NULL;
	}
	connection_data->client_transformation_fd = data->client_pipe[READ];
	connection_data->origin_transformation_fd = data->origin_pipe[WRITE];

	data->tmp_buff = new_buffer();

	char * command = global_settings.transformation_command;
	pid_t pid = fork();
	if (pid < 0) {
		send_error(500, "Internal server error", (char*) 0, "Coudn't execute transformation", connection_data);
	}

	if (pid == 0) {
		dup2(data->origin_pipe[READ], 0);
		dup2(data->client_pipe[WRITE], 1);
		close(data->origin_pipe[WRITE]);
		close(data->client_pipe[READ]);
		execl("/bin/sh", "sh", "-c", command, (char *) 0);
	}

	close(data->client_pipe[WRITE]);
	close(data->origin_pipe[READ]);
	data->client_pipe[WRITE] = -1;
	data->origin_pipe[READ] = -1;

	if(selector_fd_set_nio(data->client_pipe[READ]) == -1) {
		DieWithSystemMessage("setting transformer flags failed");
	}

	if(selector_fd_set_nio(data->origin_pipe[WRITE]) == -1) {
		DieWithSystemMessage("setting transformer flags failed");
	}

	if(SELECTOR_SUCCESS != selector_register(connection_data->s, data->client_pipe[READ], proxy_transformation_handler(),
	                                         OP_READ, data))
		DieWithSystemMessage("registering fd failed");
	if(SELECTOR_SUCCESS != selector_register(connection_data->s, data->origin_pipe[WRITE], proxy_transformation_handler(),
	                                         OP_WRITE, data))
		DieWithSystemMessage("registering fd failed");

	return data;
}

void proxy_transformation_read(struct selector_key *key) {
	proxy_transformation_data * data = key->data;
	connection_data * connection_data = data->connection_data;
	char aux[1000];
	int ret = -1;
	int bytes_to_send;

	do {
		bytes_to_send = sizeof(aux) > available_write_bytes(connection_data->origin_data) ? available_write_bytes(connection_data->origin_data) : sizeof(aux);
		if (bytes_to_send == 0) {
			/* Buffer full. Must wait for the client to read it */
			selector_set_interest_key(key, OP_NOOP);
			break;
		}
		if ((ret = read(data->client_pipe[READ], aux, bytes_to_send)) <= 0)
			break;
		write_chunked(connection_data->origin_data, aux, ret);
	} while (ret > 0);

	if (ret == 0) {
		/* Connection was closed. End of response. */
		write_chunked(connection_data->origin_data, "", 0);
		data->client_pipe[READ] = -1;
		selector_unregister_fd(key->s, key->fd);
		if (connection_data->state == TRANSFORMER_MUST_CLOSE) {
			kill_origin(connection_data);
			connection_data->state = CLOSED;
		} else
			kill_transformation(connection_data);
	}

	selector_set_interest(key->s, connection_data->client_fd, OP_WRITE);
}

void proxy_transformation_write(struct selector_key *key) {
	proxy_transformation_data * data = key->data;
	connection_data * connection_data = data->connection_data;

	char aux[1000];
	int read_ret, write_ret;
	int pipe_full = 0;

	while(pipe_full == 0 && buffer_count(data->tmp_buff) > 0) {
		buffer_reset_peek_line(data->tmp_buff);
		read_ret = buffer_peek_data(data->tmp_buff, aux, sizeof(aux));
		write_ret = write(data->origin_pipe[WRITE], aux, read_ret);
		write_ret = write_ret > 0 ? write_ret : 0;
		buffer_read_data(data->tmp_buff, aux, write_ret);
		if (write_ret < read_ret)
			pipe_full = 1;
	}

	while (pipe_full == 0 && (read_ret = read_unchunked(connection_data->origin_data, aux, sizeof(aux))) > 0) {
		write_ret = write(data->origin_pipe[WRITE], aux, read_ret);
		write_ret = write_ret > 0 ? write_ret : 0;
		if (write_ret < read_ret) {
			buffer_write_data(data->tmp_buff, aux+write_ret, read_ret-write_ret);
			pipe_full = 1;
		}
	}

	if (pipe_full == 0 && read_ret == -1) {     /* Error */
		send_error(500, "Internal server error", (char*) 0, "Unexpected body format", data->connection_data);
		return;
	}

	if (connection_data->response_has_finished == 1) {
		data->origin_pipe[WRITE] = -1;
		selector_unregister_fd(key->s, key->fd);
	}

	if (pipe_full == 0)
		selector_set_interest_key(key, OP_NOOP);

	selector_set_interest(key->s, connection_data->origin_fd, OP_READ);
}

void proxy_transformation_close(struct selector_key *key) {
	close(key->fd);
}

void kill_transformation(connection_data * connection_data) {
	proxy_transformation_data * data = connection_data->transformation_data;

	if (data == NULL)
		return;

	if (data->client_pipe[READ] != -1)
		selector_unregister_fd(connection_data->s, data->client_pipe[READ]);
	if (data->client_pipe[WRITE] != -1)
		close(data->client_pipe[WRITE]);
	if (data->origin_pipe[READ] != -1)
		close(data->origin_pipe[READ]);
	if (data->origin_pipe[WRITE] != -1)
		selector_unregister_fd(connection_data->s, data->origin_pipe[WRITE]);

	//ToDo: que hago con el proceso?

	free(data);
	connection_data->transformation_data = NULL;
	connection_data->client_transformation_fd = -1;
	connection_data->origin_transformation_fd = -1;
}

enum media_type_match_states {
	MATCH_TYPE,
	MATCH_SUBTYPE,
	SKIP_TYPE,
	SKIP_SPACES
};

int media_type_match(char * media_types, char * content_type) {
	int i, j;
	for (i = 0; content_type[i] == ' '; i++);
	content_type = content_type + i;
	enum media_type_match_states state = SKIP_SPACES;
	
	for (i = 0;; i++) {
		switch(state) {
			case MATCH_TYPE:
				if (media_types[i] == content_type[j]) {
					j++;
				}
				else {
					state = SKIP_TYPE;
					break;
				}
				if (media_types[i] == '/') {
					if (media_types[i+1] == '*')
						return 1;
					else
						state = MATCH_SUBTYPE;
				}
				break;
			case MATCH_SUBTYPE:
				if (media_types[i] == ';' || media_types[i] == ',' || media_types[i] == ' ' || media_types[i] == '\0') {
					if (content_type[j] == ';' || content_type[j] == ' ' || content_type[j] == '\0') {
						return 1;
					} else {
						state = SKIP_TYPE;
						break;
					}
				}
				if (media_types[i] == content_type[j])
					j++;
				else {
					state = SKIP_TYPE;
				}
				break;
			case SKIP_TYPE:
				if (media_types[i-1] == ',') {
					i--;
					state = SKIP_SPACES;
				}
				break;
			case SKIP_SPACES:
				if (media_types[i] != ' ') {
					if (media_types[i] == '*')
						return 1;
					i--;
					j = 0;
					state = MATCH_TYPE;
				}
				break;
		}

		if (media_types[i] == '\0' && i > 0) {
			break;
		}
	}

	return 0;
}