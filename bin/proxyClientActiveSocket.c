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
	.handle_close = proxy_client_active_socket_close,
};

fd_handler * proxy_client_active_socket_fd_handler(void){
	return &fd;
}

static void * open_origin_socket( void * clientData );

void proxy_client_active_socket_read(struct selector_key *key) {
	proxy_client_active_socket_data * data = key->data;

	int client_fd = key->fd;
	int origin_fd = data->origin_fd;

	printf("origin: %d\n", origin_fd);

	if (data->state == READING_HEADER_FIRST_LINE) {
		char aux[1000];
		int ret;
		while ((ret = recv(client_fd, aux, sizeof(aux), 0)) > 0) {
			//TODO: check buffer space
			buffer_write_data(data->write_buff, aux, ret);
		}

		if (ret == 0) {
			/* Connection was closed */
			selector_unregister_fd(key->s, key->fd);
			return;
		}

		/* Peeks the first line of the request */
		ret = buffer_peek_line(data->write_buff, aux, sizeof(aux));
		if (ret == -1)
			send_error( 400, "Bad Request", (char*) 0, "Can't parse request." );

		if (ret > 0) {
			char method[10000], url[10000], protocol[10000], host[10000], path[10000];
			unsigned short port;
			int iport;
			pthread_t tid;
			int ssl;

			/* Parse it. */
			trim( aux );
			if ( sscanf( aux, "%[^ ] %[^ ] %[^ ]", method, url, protocol ) != 3 )
				send_error( 400, "Bad Request", (char*) 0, "Can't parse request." );

			if ( url[0] == '\0' )
				send_error( 400, "Bad Request", (char*) 0, "Null URL." );

			if ( strncasecmp( url, "http://", 7 ) == 0 ) {
				(void) strncpy( url, "http", 4 );                                                                                                 /* making sure it's lower case */
				if ( sscanf( url, "http://%[^:/]:%d%s", host, &iport, path ) == 3 )
					port = (unsigned short) iport;
				else if ( sscanf( url, "http://%[^/]%s", host, path ) == 2 )
					port = 80;
				else if ( sscanf( url, "http://%[^:/]:%d", host, &iport ) == 2 ) {
					port = (unsigned short) iport;
					*path = '\0';
				} else if ( sscanf( url, "http://%[^/]", host ) == 1 ) {
					port = 80;
					*path = '\0';
				} else
					send_error( 400, "Bad Request", (char*) 0, "Can't parse URL." );
				 ssl = 0;
			} else if ( strcmp( method, "CONNECT" ) == 0 ) {
				if ( sscanf( url, "%[^:]:%d", host, &iport ) == 2 )
	    			port = (unsigned short) iport;
				else if ( sscanf( url, "%s", host ) == 1 )
	    			port = 443;
				else
	    			send_error( 400, "Bad Request", (char*) 0, "Can't parse URL." );
				ssl = 1;
			} else
				send_error( 400, "Bad Request", (char*) 0, "Unknown URL type." );

			data->hostname = host;
			data->port = port;

			struct selector_key* k = malloc(sizeof(*key));
			if(k == NULL) {
				send_error( 500, "Internal server error", (char*) 0, "Ran out of memory" );
			}
			memcpy(k, key, sizeof(*k));

			/* Open socket to the origin server. */
			if(-1 == pthread_create(&tid, 0, open_origin_socket, k)) {
				send_error( 500, "Internal server error", (char*) 0, "Couldn't process request" );
			} else {
				selector_set_interest_key(key, OP_NOOP);
			}

			data->state = CONNECTING;
		}
	}

	if (data->state == PASSING_CONTENT) {
		char aux[1000];
		int ret;
		int bytes_to_send;

		do {
			bytes_to_send = sizeof(aux) > buffer_space(data->write_buff) ? buffer_space(data->write_buff) : sizeof(aux);
			if (bytes_to_send == 0) {
				selector_set_interest_key(key, OP_NOOP);
				break;
			}
			ret = recv(client_fd, aux, bytes_to_send, 0);
			buffer_write_data(data->write_buff, aux, ret);
		} while (ret > 0);

		if (ret == 0) {
			/* Connection was closed */
			selector_unregister_fd(key->s, key->fd);
			return;
		}

		selector_set_interest(key->s, origin_fd, OP_WRITE);
	}
}

void proxy_client_active_socket_write(struct selector_key *key) {
	proxy_client_active_socket_data * data = key->data;
	int origin_fd = data->origin_fd;

	char aux[1000];
	int ret;
	while ((ret = buffer_read_data(data->read_buff, aux, sizeof(aux))) > 0) {
		//TODO: checkear que se mandaron todos los bytes
		send(key->fd, aux, ret, 0);
	}

	selector_set_interest_key(key, OP_READ);
	selector_set_interest(key->s, origin_fd, OP_READ);
}

static void * open_origin_socket( void * clientKey ) {
	struct selector_key * key = clientKey;
	proxy_client_active_socket_data * data = key->data;
	int sockfd;
	char portString[MAX_PORTSTRING_SIZE];
	sprintf(portString, "%d", data->port);

	/* No need to join the thread */
	pthread_detach(pthread_self());

	/* Tell the system what kind(s) of address info we want */
	struct addrinfo addrCriteria;                                 // Criteria for address match
	memset(&addrCriteria, 0, sizeof(addrCriteria));               // Zero out structure
	addrCriteria.ai_family = AF_UNSPEC;                           // Any address family
	addrCriteria.ai_socktype = SOCK_STREAM;                       // Only stream sockets
	addrCriteria.ai_protocol = IPPROTO_TCP;                       // Only TCP protocol

	/* Get address(es) associated with the specified name/service */
	struct addrinfo *addrList;                 // Holder for list of addresses returned

	/* Modify servAddr contents to reference linked list of addresses */
	int rtnVal = getaddrinfo(data->hostname, portString, &addrCriteria, &addrList);
	if (rtnVal != 0)
		send_error( 404, "Not Found", (char*) 0, "Unknown host." );

	int connectRet;
	/* Iterate over returned addresses */
	for (struct addrinfo *addr = addrList; addr != NULL; addr = addr->ai_next) {
		sockfd = socket( addr->ai_family, addr->ai_socktype, addr->ai_protocol );
		if (sockfd < 0) {
			send_error( 500, "Internal Error", (char*) 0, "Couldn't create socket." );
			continue;
		}
		if ((connectRet = connect(sockfd, addr->ai_addr, addr->ai_addrlen)) < 0) {
			close(sockfd);
			continue;
		}
		/* Connected succesfully! */
		break;
	}

	freeaddrinfo(addrList);                     // Free addrinfo allocated in getaddrinfo()

	if (connectRet < 0)
		send_error( 503, "Service Unavailable", (char*) 0, "Connection refused." );
	else
		data->origin_fd = sockfd;

	selector_notify_block(key->s, key->fd);

	return 0;
}

void proxy_client_active_socket_block(struct selector_key *key) {
	proxy_client_active_socket_data * data = key->data;
	data->state = PASSING_CONTENT;
	selector_set_interest_key(key, OP_READ);

	if(selector_fd_set_nio(data->origin_fd) == -1) {
		DieWithSystemMessage("setting origin server flags failed");
	}

	//ToDO: mover a proxyOriginActiveSocket
	proxy_origin_active_socket_data * originData = malloc(sizeof(proxy_origin_active_socket_data));
	originData->client_fd = key->fd;
	originData->read_buff = data->write_buff;
	originData->write_buff = data->read_buff;
	originData->closed = 0;

	if(SELECTOR_SUCCESS != selector_register(key->s, data->origin_fd, proxy_origin_active_socket_fd_handler(),
	                                         OP_WRITE, originData)) {
		DieWithSystemMessage("registering fd failed");
	}
}

void proxy_client_active_socket_close(struct selector_key *key) {
	proxy_client_active_socket_data * data = key->data;
	if (data->closed == 1)
		return;
	data->closed = 1;
	selector_unregister_fd(key->s, data->origin_fd);
	buffer_free(data->write_buff);
	free(data);
	close(key->fd);
}
