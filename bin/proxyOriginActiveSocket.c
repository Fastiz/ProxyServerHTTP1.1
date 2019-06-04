#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "include/proxyOriginActiveSocket.h"
#include "include/helpers.h"

static fd_handler fd = {
	.handle_read = proxy_origin_active_socket_read,
	.handle_write = proxy_origin_active_socket_write,
	.handle_block = proxy_origin_active_socket_block,
	.handle_close = proxy_origin_active_socket_close,
};

fd_handler * proxy_origin_active_socket_fd_handler(void){
	return &fd;
}

/**
 *  Contains buffers and client data
 */
void * proxy_origin_active_socket_data_init(int client_fd, buffer * read, buffer * write) {
	proxy_origin_active_socket_data * data = malloc(sizeof(proxy_origin_active_socket_data));
	data->client_fd = client_fd;
	data->read_buff = read;
	data->write_buff = write;
	data->closed = 0;
	return data;
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
	int ret;
	int bytes_to_send;
	do {
		bytes_to_send = sizeof(aux) > buffer_space(data->write_buff) ? buffer_space(data->write_buff) : sizeof(aux);
		if (bytes_to_send == 0) {
			selector_set_interest_key(key, OP_NOOP);
			break;
		}
		ret = recv(origin_fd, aux, bytes_to_send, 0);
		buffer_write_data(data->write_buff, aux, ret);
	} while (ret > 0);

	if (ret == 0) {
		/* Connection was closed */
		selector_unregister_fd(key->s, key->fd);
		return;
	}

	selector_set_interest(key->s, client_fd, OP_WRITE);
}

void proxy_origin_active_socket_block(struct selector_key *key) {

}

void proxy_origin_active_socket_close(struct selector_key *key) {
	proxy_origin_active_socket_data * data = key->data;
	if (data->closed == 1)
		return;
	data->closed = 1;
	selector_unregister_fd(key->s, data->client_fd);
	buffer_free(data->write_buff);
	free(data);
	close(key->fd);
}
