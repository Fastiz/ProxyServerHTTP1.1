#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "include/proxyOriginActiveSocket.h"
#include "include/helpers.h"
#include "include/proxyTransformation.h"

static fd_handler fd = {
	.handle_read = proxy_origin_active_socket_read,
	.handle_write = proxy_origin_active_socket_write,
	.handle_block = proxy_origin_active_socket_block,
	.handle_close = proxy_origin_active_socket_close
};

fd_handler * proxy_origin_active_socket_fd_handler(void){
	return &fd;
}

#define CHUNK_SIZE 100

enum parser_states {
	READING_HEADER,
	READING_BODY,
	SSL_CONNECTION
};

typedef struct {
	enum parser_states state;
	int responseHasFinished;
	int contentLength;
	int chunk_size;
	int chunk_bytes;
} parser_data;

/**
 *  Contains buffers and client data
 */
typedef struct {
	int client_fd;
	int transformation_fd;
	void * client_data;
	int ssl;
	buffer * read_buff;
	buffer * write_buff;
	buffer * parser_buff;
	parser_data * parser_data;
	int closed;
} proxy_origin_active_socket_data;

void parse_content(proxy_origin_active_socket_data * data, struct selector_key * key);

void reset_origin_data(fd_selector s, void * origin_data, int origin_fd) {
	proxy_origin_active_socket_data * data = origin_data;
	parser_data * parser_data = data->parser_data;
	buffer_clean(data->parser_buff);
	parser_data->contentLength = -1;
	parser_data->chunk_size = -1;
	parser_data->chunk_bytes = -1;
	parser_data->responseHasFinished = 0;

	if (data->ssl == 1) {
		parser_data->state = SSL_CONNECTION;
	} else {
		parser_data->state = READING_HEADER;
		proxy_transformation_data_init(s, data->client_data, data, data->client_fd, origin_fd);
	}
}

void * proxy_origin_active_socket_data_init(fd_selector s, int ssl, int client_fd, int origin_fd, void * client_data, buffer * read, buffer * write) {
	proxy_origin_active_socket_data * data = malloc(sizeof(proxy_origin_active_socket_data));
	data->client_fd = client_fd;
	data->client_data = client_data;
	data->transformation_fd = -1;
	data->ssl = ssl;
	data->read_buff = read;
	data->write_buff = write;
	data->parser_buff = new_buffer();
	data->closed = 0;
	data->parser_data = malloc(sizeof(parser_data));
	reset_origin_data(s, data, origin_fd);

	return data;
}

void set_origin_transformation_fd(void * origin_data, int fd) {
	proxy_origin_active_socket_data * data = origin_data;
	data->transformation_fd = fd;
}

void proxy_origin_active_socket_write(struct selector_key *key){
	proxy_origin_active_socket_data * data = key->data;
	int client_fd = data->client_fd;

	char aux[1000];
	int ret;
	while ((ret = buffer_read_data(data->read_buff, aux, sizeof(aux))) > 0) {
		//TODO: checkear que se mandaron todos los bytes
		send(key->fd, aux, ret, 0);
	}

	selector_set_interest_key(key, OP_READ);
	selector_set_interest(key->s, client_fd, OP_READ);
}

void proxy_origin_active_socket_read(struct selector_key *key) {
	proxy_origin_active_socket_data * data = key->data;
	int client_fd = data->client_fd;
	int origin_fd = key->fd;

	char aux[1000];
	int ret = -1;
	int bytes_to_send;
	do {
		bytes_to_send = sizeof(aux) > available_response_bytes(data) ? available_response_bytes(data) : sizeof(aux);
		if (bytes_to_send == 0) {
			/* Buffer full. Must wait for the client/transformer to read it */
			selector_set_interest_key(key, OP_NOOP);
			break;
		}
		ret = recv(origin_fd, aux, bytes_to_send, 0);
		/* reset the connection if necessary */
		if (ret > 0 && response_has_finished(data) == 1)
			reset_origin_data(key->s, data, key->fd);		
		buffer_write_data(data->parser_buff, aux, ret);
	} while (ret > 0);

	parse_content(data, key);

	if (ret == 0) {
		/* Connection was closed */
		selector_unregister_fd(key->s, key->fd);
		return;
	}
}

int available_response_bytes(void * origin_data) {
	proxy_origin_active_socket_data * data = origin_data;
	parser_data * parser_data = data->parser_data;
	//if (parser_data->state == READING_HEADER) {
	int ret = buffer_space(data->write_buff) > buffer_space(data->parser_buff) ? buffer_space(data->parser_buff) : buffer_space(data->write_buff);
	/* The response will likely get longer due to the chunked encoding */
	return (ret - 80) > 0 ? ret - 80 : 0;
	//}
}

void parse_content(proxy_origin_active_socket_data * data, struct selector_key * key) {
	parser_data * parser_data = data->parser_data;
	char aux[1000];
	int ret;

	if (parser_data->state == READING_HEADER) {
		while ((ret = buffer_peek_line(data->parser_buff, aux, sizeof(aux))) > 0) {
			buffer_read_data(data->parser_buff, aux, ret);

			if (strncasecmp(aux, "Content-Length:", 15) == 0) {
				int contentLength;
				/* Space after ':' is optional */
				if (sscanf(aux + 15, "%d", &contentLength) == 1 || sscanf(aux + 15, " %d", &contentLength) == 1) {
					parser_data->contentLength = contentLength;
					buffer_write_data(data->write_buff, "Transfer-Encoding: chunked\n", 27);
				}
				continue;
			}

			//TODO: que pasa si no encuentro chunked ni context length
			buffer_write_data(data->write_buff, aux, ret);
			aux[ret] = 0;

			if (strcmp(aux, "\n") == 0 || strcmp(aux, "\r\n") == 0) {
				parser_data->state = READING_BODY;
				break;
			}
		}

		selector_set_interest(key->s, data->client_fd, OP_WRITE);
	}

	if (parser_data->state == READING_BODY) {
		if (data->transformation_fd == -1) {
			while ((ret = read_unchunked(data, aux, sizeof(aux))) > 0) {
				//buffer_write_data(data->write_buff, aux, ret);
				write_chunked(data, aux, ret);
			}

			if (response_has_finished(data) == 1) {
				/* Write a 0-byte chunk to indicate end of body */
				write_chunked(data, "", 0);
			}

			selector_set_interest(key->s, data->client_fd, OP_WRITE);
		} else {
			selector_set_interest(key->s, data->transformation_fd, OP_WRITE);
		}
	}

	if (parser_data->state == SSL_CONNECTION) {
		while ((ret = buffer_read_data(data->parser_buff, aux, sizeof(aux))) > 0) {
			buffer_write_data(data->write_buff, aux, ret);
		}

		selector_set_interest(key->s, data->client_fd, OP_WRITE);
	}
}

int response_has_finished(void * origin_data) {
	proxy_origin_active_socket_data * data = origin_data;
	return data->parser_data->responseHasFinished;
}

int read_unchunked(void * origin_data, char * dest_buff, int size) {
	proxy_origin_active_socket_data * data = origin_data;
	parser_data * parser_data = data->parser_data;
	int ret;

	if (parser_data->contentLength != -1) {
		ret = buffer_read_data(data->parser_buff, dest_buff, size);
		parser_data->contentLength -= ret;
		if (parser_data->contentLength == 0)
			parser_data->responseHasFinished = 1;
		//TODO: Si leo mas bytes que el context length es un internal server error!
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
					send_error(500, "Internal server error", (char*) 0, "Unexpected body format");

				if (ret1 > 0) {
					if (strcmp(endOfChunk, "\r\n") != 0)
						send_error(500, "Internal server error", (char*) 0, "Unexpected body format");

					buffer_read_data(data->parser_buff, aux, ret1);
					parser_data->chunk_bytes = -1;

					if(parser_data->chunk_size == 0)
						parser_data->responseHasFinished = 1;
				}
			}

			if (parser_data->chunk_bytes == -1) {
				char beginningOfChunk[12], hex[10];

				buffer_reset_peek_line(data->parser_buff);
				ret2 = buffer_peek_line(data->parser_buff, beginningOfChunk, sizeof(beginningOfChunk));

				if (ret2 == -1)
					send_error(500, "Internal server error", (char*) 0, "Unexpected body format");


				if (ret2 > 0) {
					if (sscanf(beginningOfChunk, "%9s\r\n", hex) != 1)
						send_error(500, "Internal server error", (char*) 0, "Unexpected body format");

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

//todo: deberia retornar int?
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
	/*proxy_origin_active_socket_data * data = key->data;
	   if (data->closed == 1)
	        return;
	   data->closed = 1;
	   selector_unregister_fd(key->s, data->client_fd);
	   buffer_free(data->write_buff);
	   free(data);
	   close(key->fd);*/
}
