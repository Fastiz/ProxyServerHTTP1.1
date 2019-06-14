#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include "include/buffer.h"
#include "include/selector.h"
#include "include/helpers.h"
#include "include/proxyPassiveSocket.h"
#include "include/proxyClientActiveSocket.h"
#include "include/proxyOriginActiveSocket.h"

#define MAX_PORTSTRING_SIZE 10

static fd_handler fd = {
	.handle_read = proxy_client_active_socket_read,
	.handle_write = proxy_client_active_socket_write,
	.handle_block = proxy_client_active_socket_block,
	.handle_close = proxy_client_active_socket_close
};

fd_handler * proxy_client_active_socket_fd_handler(void){
	return &fd;
}

enum client_states {
	/**
	 *  Reads the first line (explicit mode) or looks
	 *  for the Host header (transparent mode) in the first request
	 */
	READING_HEADER_FIRST_LINE,

	/**
	 *  Searchs for host header in html request.
	 */
	SEARCHING_FOR_HEADER_HOST,

	/**
	 *  Waits for the DNS request to resolve
	 */
	CONNECTING,

	/**
	 *  Transfers bits from the client to the origin server
	 */
	PASSING_CONTENT
};

enum ssl_states {
	/**
	 *  Connection doesn't use SSL
	 */
	NO_SSL,

	/**
	 *  Awaiting connection to the origin server
	 */
	SSL_CONNECTING,

	/**
	 *  Succesfully connected to the origin server through SSL
	 */
	SSL_OK
};

/**
 *  Contains buffers and connection data
 */
typedef struct proxy_client_active_socket_data {
	connection_data * connection_data;
	char* hostname;
	unsigned short port;
	buffer * write_buff;
	buffer * read_buff;
	enum client_states state;
	enum ssl_states ssl;
} proxy_client_active_socket_data;


static void * open_origin_socket(void * client_key);
static void process_ssl (struct selector_key *key);

void * proxy_client_active_socket_data_init(fd_selector s, int client_fd) {
	proxy_client_active_socket_data * data = malloc(sizeof(proxy_client_active_socket_data));
	data->state = READING_HEADER_FIRST_LINE;
	data->read_buff = new_buffer();
	data->write_buff = new_buffer();

	connection_data * conn_data = malloc(sizeof(connection_data));
	data->connection_data = conn_data;
	conn_data->s = s;
	conn_data->client_fd = client_fd;
	conn_data->client_data = data;
	conn_data->origin_fd = -1;
	conn_data->origin_data = NULL;
	conn_data->origin_transformation_fd = -1;
	conn_data->client_transformation_fd = -1;
	conn_data->transformation_data = NULL;
	conn_data->state = ALIVE;

	return data;
}

void proxy_client_active_socket_read(struct selector_key *key) {
	proxy_client_active_socket_data * data = key->data;
	connection_data * connection_data = data->connection_data;

	int client_fd = connection_data->client_fd;
	int origin_fd = connection_data->origin_fd;

	if (data->state == READING_HEADER_FIRST_LINE) {
		char aux[1000];
		int ret;
		while ((ret = recv(client_fd, aux, sizeof(aux), 0)) > 0) {
			//TODO: check buffer space
			buffer_write_data(data->write_buff, aux, ret);
		}

		if (ret == 0) {
			/* Connection was closed */
			kill_client(connection_data);
			return;
		}

		/* Peeks the first line of the request */
		ret = buffer_peek_line(data->write_buff, aux, sizeof(aux));
		if (ret == -1) {
			send_error(400, "Bad Request", (char*) 0, "Can't parse request.", connection_data);
			return;
		}

		if (ret > 0) {
			char method[10000], url[10000], protocol[10000], host[10000], path[10000];
			unsigned short port;
			int iport;
			pthread_t tid;
			int ssl;

			/* Parse it. */
			trim(aux);
			if (sscanf(aux, "%[^ ] %[^ ] %[^ ]", method, url, protocol ) == 3 && url[0]!='/') {
				if (url[0] == '\0') {
					send_error(400, "Bad Request", (char*) 0, "Null URL.", connection_data);
					return;
				}

				if (strncasecmp(url, "http://", 7) == 0) {
					strncpy(url, "http", 4);                                                                                                                 /* making sure it's lower case */
					if (sscanf(url, "http://%[^:/]:%d%s", host, &iport, path) == 3)
						port = (unsigned short) iport;
					else if (sscanf(url, "http://%[^/]%s", host, path) == 2)
						port = 80;
					else if (sscanf(url, "http://%[^:/]:%d", host, &iport) == 2) {
						port = (unsigned short) iport;
						*path = '\0';
					} else if (sscanf(url, "http://%[^/]", host) == 1) {
						port = 80;
						*path = '\0';
					} else {
						send_error(400, "Bad Request", (char*) 0, "Can't parse URL.", connection_data);
						return;
					}
					ssl = NO_SSL;
				} else if (strcmp(method, "CONNECT") == 0) {
					if (sscanf(url, "%[^:]:%d", host, &iport) == 2)
						port = (unsigned short) iport;
					else if (sscanf(url, "%s", host) == 1)
						port = 443;
					else {
						send_error(400, "Bad Request", (char*) 0, "Can't parse URL.", connection_data);
						return;
					}
					ssl = SSL_CONNECTING;
				} else {
					send_error(400, "Bad Request", (char*) 0, "Unknown URL type.", connection_data);
					return;
				}

				data->hostname = host;
				data->port = port;
				data->ssl = ssl;

				struct selector_key* k = malloc(sizeof(*key));
				if(k == NULL) {
					send_error(500, "Internal server error", (char*) 0, "Ran out of memory", connection_data);
					return;
				}
				memcpy(k, key, sizeof(*k));

				/* Open socket to the origin server. */
				if(-1 == pthread_create(&tid, 0, open_origin_socket, k)) {
					send_error(500, "Internal server error", (char*) 0, "Couldn't process request", connection_data);
					return;
				} else {
					selector_set_interest_key(key, OP_NOOP);
				}

				data->state = CONNECTING;

				printf("Nuevo request hacia: %s:%d\n", host, port);
			} else {
				data->state = SEARCHING_FOR_HEADER_HOST;
				data->ssl = NO_SSL;
			}
		}
	}

	//TODO: client_fd could be empty if comes from state 'READING_HEADER_FIRST_LINE'
	if(data->state == SEARCHING_FOR_HEADER_HOST) {
		char aux[1000];
		int ret;
		while ((ret = recv(client_fd, aux, sizeof(aux), 0)) > 0) {
			//TODO: check buffer space
			buffer_write_data(data->write_buff, aux, ret);
		}
		if (ret == 0) {
			/* Connection was closed */
			kill_client(connection_data);
			return;
		}

		/* Peeks the first line of the request */
		ret = buffer_peek_line(data->write_buff, aux, sizeof(aux));
		if (ret == -1) {
			send_error(400, "Bad Request", (char*) 0, "Can't parse request.", connection_data);
			return;
		}

		if(ret > 0) {
			pthread_t tid;
			unsigned short port;
			int iport;
			char host[1000], host_header[1000], path[1000];

			trim(aux);

			lineToLowerCase(aux, ret);

			if(sscanf(aux, "host: %s", host_header)==1) {
				if (sscanf(host_header, "%[^:/]:%d%s", host, &iport, path) == 3)
					port = (unsigned short) iport;
				else if (sscanf(host_header, "%[^/]%s", host, path) == 2)
					port = 80;
				else if (sscanf(host_header, "%[^:/]:%d", host, &iport) == 2) {
					port = (unsigned short) iport;
					*path = '\0';
				} else if (sscanf(host_header, "%[^/]", host ) == 1) {
					port = 80;
					*path = '\0';
				} else {
					send_error(400, "Bad Request", (char*) 0, "Can't parse URL.", connection_data);
					return;
				}

				data->hostname = host;
				data->port = port;

				struct selector_key* k = malloc(sizeof(*key));
				if(k == NULL) {
					send_error(500, "Internal server error", (char*) 0, "Ran out of memory", connection_data);
					return;
				}
				memcpy(k, key, sizeof(*k));

				/* Open socket to the origin server. */
				if(-1 == pthread_create(&tid, 0, open_origin_socket, k)) {
					send_error(500, "Internal server error", (char*) 0, "Couldn't process request", connection_data);
					return;
				} else {
					selector_set_interest_key(key, OP_NOOP);
				}

				data->state = CONNECTING;

				printf("Nuevo request hacia: %s:%d\n", host, port);
			}
		}

	}

	if (data->state == PASSING_CONTENT) {
		if (data->ssl == SSL_CONNECTING) {
			process_ssl(key);
			return;
		}

		char aux[1000];
		int ret;
		int bytes_to_send;

		do {
			bytes_to_send = sizeof(aux) > buffer_space(data->write_buff) ? buffer_space(data->write_buff) : sizeof(aux);
			if (bytes_to_send == 0) {
				/* Buffer full. Must wait for the origin server to read it */
				selector_set_interest_key(key, OP_NOOP);
				break;
			}
			ret = recv(client_fd, aux, bytes_to_send, 0);
			buffer_write_data(data->write_buff, aux, ret);
		} while (ret > 0);

		if (ret == 0) {
			/* Connection was closed */
			kill_client(connection_data);
			return;
		}

		selector_set_interest(key->s, origin_fd, OP_WRITE);
	}
}

static void process_ssl (struct selector_key *key) {
	proxy_client_active_socket_data * data = key->data;
	char aux[1000];
	int ret;

	if (data->ssl == SSL_CONNECTING) {
		do {
			ret = buffer_peek_line(data->write_buff, aux, sizeof(aux));
			buffer_read_data(data->write_buff, aux, ret);
			aux[ret] = 0;
		} while (ret > 0 && strcmp( aux, "\n" ) != 0 && strcmp( aux, "\r\n" ) != 0 );

		if (ret > 0) {
			char * str = "HTTP/1.0 200 Connection established\r\n\r\n";
			send(key->fd, str, strlen(str), 0);
			buffer_clean(data->write_buff);
			data->ssl = SSL_OK;
		}
	}
}

void proxy_client_active_socket_write(struct selector_key *key) {
	proxy_client_active_socket_data * data = key->data;
	connection_data * connection_data = data->connection_data;

	char aux[1000];
	int ret;
	while ((ret = buffer_read_data(data->read_buff, aux, sizeof(aux))) > 0) {
		//TODO: checkear que se mandaron todos los bytes
		send(key->fd, aux, ret, 0);
	}

	if(connection_data->state == CLOSED) {
		kill_client(connection_data);
	}

	selector_set_interest_key(key, OP_READ);

	if (connection_data->client_transformation_fd == -1) {
		selector_set_interest(key->s, connection_data->origin_fd, OP_READ);
	} else {
		selector_set_interest(key->s, connection_data->client_transformation_fd, OP_READ);
	}
}

static void * open_origin_socket(void * client_key) {
	struct selector_key * key = client_key;
	proxy_client_active_socket_data * data = key->data;
	connection_data * connection_data = data->connection_data;

	int sockfd;
	char portString[MAX_PORTSTRING_SIZE];
	sprintf(portString, "%d", data->port);

	/* No need to join the thread */
	pthread_detach(pthread_self());

	/* Tell the system what kind(s) of address info we want */
	struct addrinfo addrCriteria;                                         // Criteria for address match
	memset(&addrCriteria, 0, sizeof(addrCriteria));                       // Zero out structure
	addrCriteria.ai_family = AF_UNSPEC;                                   // Any address family
	addrCriteria.ai_socktype = SOCK_STREAM;                               // Only stream sockets
	addrCriteria.ai_protocol = IPPROTO_TCP;                               // Only TCP protocol

	/* Get address(es) associated with the specified name/service */
	struct addrinfo *addrList;                         // Holder for list of addresses returned

	/* Modify servAddr contents to reference linked list of addresses */
	int rtnVal = getaddrinfo(data->hostname, portString, &addrCriteria, &addrList);
	if (rtnVal != 0) {
		send_error(404, "Not Found", (char*) 0, "Unknown host.", connection_data);
		return 0;
	}

	int connectRet;
	/* Iterate over returned addresses */
	for (struct addrinfo *addr = addrList; addr != NULL; addr = addr->ai_next) {
		sockfd = socket( addr->ai_family, addr->ai_socktype, addr->ai_protocol );
		if (sockfd < 0) {
			continue;
		}
		if ((connectRet = connect(sockfd, addr->ai_addr, addr->ai_addrlen)) < 0) {
			close(sockfd);
			continue;
		}
		/* Connected succesfully! */
		break;
	}

	freeaddrinfo(addrList);             // Free addrinfo allocated in getaddrinfo()

	if (connectRet < 0) {
		send_error(503, "Service Unavailable", (char*) 0, "Connection refused.", connection_data);
		return 0;
	} else
		connection_data->origin_fd = sockfd;

	selector_notify_block(key->s, key->fd);

	return 0;
}

void proxy_client_active_socket_block(struct selector_key *key) {
	proxy_client_active_socket_data * data = key->data;
	connection_data * connection_data = data->connection_data;

	data->state = PASSING_CONTENT;
	selector_set_interest_key(key, OP_READ);

	if(selector_fd_set_nio(connection_data->origin_fd) == -1) {
		DieWithSystemMessage("setting origin server flags failed");
	}

	connection_data->ssl = data->ssl == NO_SSL ? 0 : 1;
	void * origin_data = proxy_origin_active_socket_data_init(connection_data, data->write_buff, data->read_buff);
	connection_data->origin_data = origin_data;

	if (data->ssl == SSL_CONNECTING) {
		buffer_reset_peek_line(data->write_buff);
		process_ssl(key);
	}

	if(SELECTOR_SUCCESS != selector_register(key->s, connection_data->origin_fd, proxy_origin_active_socket_fd_handler(),
	                                         OP_WRITE, origin_data)) {
		DieWithSystemMessage("registering fd failed");
	}
}

void proxy_client_active_socket_close(struct selector_key *key) {
	close(key->fd);
}

void kill_client(connection_data * connection_data) {
	proxy_client_active_socket_data * data = connection_data->client_data;
	buffer_free(data->write_buff);
	buffer_free(data->read_buff);
	kill_origin(connection_data);
	selector_unregister_fd(connection_data->s, connection_data->client_fd);
	free(data);
}
