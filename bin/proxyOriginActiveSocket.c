#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "include/helpers.h"
#include "include/proxyOriginActiveSocket.h"
#include "include/proxyTransformation.h"
#include "include/proxySettings.h"

static fd_handler fd = {
	.handle_read = proxy_origin_active_socket_read,
	.handle_write = proxy_origin_active_socket_write,
	.handle_block = proxy_origin_active_socket_block,
	.handle_close = proxy_origin_active_socket_close
};

fd_handler * proxy_origin_active_socket_fd_handler(void){
	return &fd;
}

enum parser_states {
	READING_HEADER,
	READING_BODY,
	SSL_CONNECTION
};

typedef struct {
	enum parser_states state;
	int content_length;
	int chunk_size;
	int chunk_bytes;
	int chunk_enabled;
} parser_data;

/**
 *  Contains buffers and client data
 */
typedef struct {
	connection_data * connection_data;
	buffer * read_buff;
	buffer * write_buff;
	buffer * parser_buff;
	parser_data * parser_data;
} proxy_origin_active_socket_data;

int parse_content(proxy_origin_active_socket_data * data, struct selector_key * key);

extern proxy_settings global_settings;

void reset_origin_data(connection_data * connection_data) {
	proxy_origin_active_socket_data * data = connection_data->origin_data;
	parser_data * parser_data = data->parser_data;

	buffer_clean(data->parser_buff);
	parser_data->content_length = -1;
	parser_data->chunk_size = -1;
	parser_data->chunk_bytes = -1;
	parser_data->chunk_enabled = 0;
	connection_data->response_has_finished = 0;
	connection_data->status_code = -1;
	connection_data->gzip = 0;

	kill_transformation(connection_data);

	if (connection_data->ssl == 1) {
		parser_data->state = SSL_CONNECTION;
		connection_data->transformation_data = NULL;
	} else {
		parser_data->state = READING_HEADER;
	}
}

void * proxy_origin_active_socket_data_init(connection_data * connection_data, buffer * read, buffer * write) {
	proxy_origin_active_socket_data * data = malloc(sizeof(proxy_origin_active_socket_data));
	if (data == NULL)
		return NULL;

	data->read_buff = read;
	data->write_buff = write;
	data->parser_buff = new_buffer();
	data->parser_data = malloc(sizeof(parser_data));
	if (data->parser_data == NULL) {
		free (data);
		return NULL;
	}

	data->connection_data = connection_data;
	connection_data->origin_data = data;
	connection_data->origin_transformation_fd = -1;
	connection_data->transformation_data = NULL;
	reset_origin_data(connection_data);

	return data;
}

void proxy_origin_active_socket_write(struct selector_key *key){
	proxy_origin_active_socket_data * data = key->data;
	connection_data * connection_data = data->connection_data;

	int client_fd = connection_data->client_fd;

	char aux[1000];
	int ret;
	while ((ret = buffer_read_data(data->read_buff, aux, sizeof(aux))) > 0) {
		//TODO: checkear que se mandaron todos los bytes
		send(key->fd, aux, ret, MSG_NOSIGNAL);
	}

	selector_set_interest_key(key, OP_READ);
	selector_set_interest(key->s, client_fd, OP_READ);
}

int i = 0;

void proxy_origin_active_socket_read(struct selector_key *key) {
	proxy_origin_active_socket_data * data = key->data;
	connection_data * connection_data = data->connection_data;
	parser_data * parser_data = data->parser_data;

	int origin_fd = key->fd;

	char aux[1000];
	int ret = -1;
	int bytes_to_send;
	int dataToParse = 0;
	do {
		bytes_to_send = sizeof(aux) > available_parser_bytes(data) ? available_parser_bytes(data) : sizeof(aux);
		if (bytes_to_send == 0) {
			/* Buffer full. Must wait for the client/transformer to read it */
			selector_set_interest_key(key, OP_NOOP);
			break;
		}
		ret = recv(origin_fd, aux, bytes_to_send, 0);
		/* Reset the connection if necessary */
		if (ret > 0) {
			dataToParse = 1;
			if (connection_data->response_has_finished == 1)
				reset_origin_data(connection_data);
		}
		
		buffer_write_data(data->parser_buff, aux, ret);
	} while (ret > 0);

	if (dataToParse == 1) {
		if (parse_content(data, key) == -1) {
			send_error(500, "Internal server error", (char*) 0, "Unexpected body format", data->connection_data);
			return;
		}
	}

	if (ret == 0) {
		/* Connection was closed */
		if (connection_data->origin_transformation_fd == -1) {
			/* No content length nor chunk encoding */
			if (parser_data->chunk_enabled == 0 && parser_data->content_length == -1)
				write_chunked(data, "", 0);
			connection_data->state = CLOSED;
			kill_origin(connection_data);
			selector_set_interest(key->s, connection_data->client_fd, OP_WRITE);
		} else {
			/* Make sure the transformation knows that the origin server has closed */
			connection_data->response_has_finished = 1;
			selector_set_interest(key->s, connection_data->origin_transformation_fd, OP_WRITE);
			connection_data->state = TRANSFORMER_MUST_CLOSE;
		}
		selector_unregister_fd(key->s, key->fd);
	}
}

int available_parser_bytes(void * origin_data) {
	proxy_origin_active_socket_data * data = origin_data;
	int ret = buffer_space(data->write_buff) > buffer_space(data->parser_buff) ? buffer_space(data->parser_buff) : buffer_space(data->write_buff);
	/* The response will likely get longer due to the chunked encoding */
	return (ret - 80) > 0 ? ret - 80 : 0;
}

int available_write_bytes(void * origin_data) {
	proxy_origin_active_socket_data * data = origin_data;
	int ret = buffer_space(data->write_buff);
	/* The response will likely get longer due to the chunked encoding */
	return (ret - 80) > 0 ? ret - 80 : 0;
}

int parse_content(proxy_origin_active_socket_data * data, struct selector_key * key) {
	parser_data * parser_data = data->parser_data;
	connection_data * connection_data = data->connection_data;

	char aux[1000];
	int ret;

	if (parser_data->state == READING_HEADER) {
		while ((ret = buffer_peek_line(data->parser_buff, aux, sizeof(aux))) > 0) {
			buffer_read_data(data->parser_buff, aux, ret);
			aux[ret] = 0;

			if (strncasecmp(aux, "Content-Length:", 15) == 0) {
				int content_length;
				/* Space after ':' is optional */
				if (sscanf(aux + 15, "%d", &content_length) == 1 || sscanf(aux + 15, " %d", &content_length) == 1) {
					parser_data->content_length = content_length;
				}
				continue;
			}

			if (strcmp(aux, "\n") == 0 || strcmp(aux, "\r\n") == 0) {
				if (parser_data->chunk_enabled == 0)
					buffer_write_data(data->write_buff, "Transfer-Encoding: chunked\n", 27);
				buffer_write_data(data->write_buff, aux, ret);
				parser_data->state = READING_BODY;
				connection_data->transformation_data = proxy_transformation_data_init(connection_data);
				break;
			}
			
			buffer_write_data(data->write_buff, aux, ret);

			trim(aux);

			if (strncasecmp(aux, "Content-Type:", 13) == 0) {
				strncpy(connection_data->content_type, aux+13, sizeof(connection_data->content_type) - 1);
			}

			if (connection_data->status_code == -1) {
				int status;
				if (sscanf(aux, "%*[^ ] %d %*[^ ]", &status) == 1) {
					connection_data->status_code = status;
					if (status == 204)
						parser_data->content_length = 0;
				} else
					connection_data->status_code = 0;
			}

			if (strncasecmp(aux, "Transfer-Encoding:", 18) == 0) {
				/* Space after ':' is optional */
				if (strncasecmp(aux+18, " chunked", 8) == 0 || strncasecmp(aux+18, "chunked", 7) == 0)
					parser_data->chunk_enabled = 1;
			}

			if (strncasecmp(aux, "Content-Encoding:", 17) == 0) {
				/* Space after ':' is optional */
				if (strncasecmp(aux+17, " gzip", 5) == 0 || strncasecmp(aux+17, "gzip", 4) == 0)
					connection_data->gzip = 1;
			}
		}

		selector_set_interest(key->s, connection_data->client_fd, OP_WRITE);
	}

	if (parser_data->state == READING_BODY) {
		if (connection_data->origin_transformation_fd == -1) {
			while ((ret = read_unchunked(data, aux, sizeof(aux))) > 0) {
				write_chunked(data, aux, ret);
			}

			if (ret == -1)            /* Error */
				return -1;

			if (connection_data->response_has_finished == 1) {
				/* Write a 0-byte chunk to indicate end of body */
				write_chunked(data, "", 0);
			}

			selector_set_interest(key->s, connection_data->client_fd, OP_WRITE);
		} else {
			selector_set_interest(key->s, connection_data->origin_transformation_fd, OP_WRITE);
		}
	}

	if (parser_data->state == SSL_CONNECTION) {
		while ((ret = buffer_read_data(data->parser_buff, aux, sizeof(aux))) > 0) {
			buffer_write_data(data->write_buff, aux, ret);
		}

		selector_set_interest(key->s, connection_data->client_fd, OP_WRITE);
	}

	return 0;
}

int read_unchunked(void * origin_data, char * dest_buff, int size) {
	proxy_origin_active_socket_data * data = origin_data;
	parser_data * parser_data = data->parser_data;
	int ret;

	if (parser_data->content_length != -1) {
		ret = buffer_read_data(data->parser_buff, dest_buff, size);
		parser_data->content_length -= ret;
		if (parser_data->content_length == 0)
			data->connection_data->response_has_finished = 1;
	} else if (parser_data->chunk_enabled == 0) {
		ret = buffer_read_data(data->parser_buff, dest_buff, size);
	} else {
		int flag = 0;
		char aux[20];
		int ret1, ret2;
		ret = 0;

		while (flag == 0) {
			if (parser_data->chunk_bytes == 0) {
				char endOfChunk[2];
				buffer_reset_peek_line(data->parser_buff);
				ret1 = buffer_peek_line(data->parser_buff, endOfChunk, sizeof(endOfChunk));

				if (ret1 == -1)
					return -1;

				if (ret1 > 0) {
					if (strcmp(endOfChunk, "\r\n") != 0)
						return -1;

					buffer_read_data(data->parser_buff, aux, ret1);
					parser_data->chunk_bytes = -1;

					if(parser_data->chunk_size == 0)
						data->connection_data->response_has_finished = 1;
				}
			}

			if (parser_data->chunk_bytes == -1) {
				char beginningOfChunk[12], hex[10];

				buffer_reset_peek_line(data->parser_buff);
				ret2 = buffer_peek_line(data->parser_buff, beginningOfChunk, sizeof(beginningOfChunk));

				if (ret2 == -1)
					return -1;

				if (ret2 > 0) {
					if (sscanf(beginningOfChunk, "%9s\r\n", hex) != 1)
						return -1;

					parser_data->chunk_size = parser_data->chunk_bytes = strtol(hex, (char**)0, 16);
					buffer_read_data(data->parser_buff, aux, ret2);

					if (parser_data->chunk_size == 0)
						continue;
				}
			}

			int bytes_to_read = size > parser_data->chunk_bytes ? parser_data->chunk_bytes : size;
			int retAux = buffer_read_data(data->parser_buff, dest_buff + ret, bytes_to_read);

			size -= retAux;
			parser_data->chunk_bytes -= retAux;

			if (retAux == 0)
				flag = 1;

			ret += retAux;
		}
	}

	return ret;
}

void write_chunked(void * origin_data, char * data_buff, int size) {
	proxy_origin_active_socket_data * data = origin_data;
	char hex[10];

	sprintf(hex, "%X\r\n", size);
	buffer_write_data(data->write_buff, hex, strlen(hex));
	buffer_write_data(data->write_buff, data_buff, size);
	buffer_write_data(data->write_buff, "\r\n", 2);
}

void proxy_origin_active_socket_block(struct selector_key *key) {

}

void proxy_origin_active_socket_close(struct selector_key *key) {
	close(key->fd);
}

void kill_origin(connection_data * connection_data) {
	proxy_origin_active_socket_data * data = connection_data->origin_data;

	if (data == NULL)
		return;

	free(data->parser_data);
	free(data->parser_buff);
	kill_transformation(connection_data);

	if (connection_data->state == ALIVE)
		selector_unregister_fd(connection_data->s, connection_data->origin_fd);

	free(data);
	connection_data->origin_data = NULL;
	connection_data->origin_fd = -1;
}
