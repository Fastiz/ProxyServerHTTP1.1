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
#include "include/selector.h"
#include "include/helpers.h"
#include "include/proxyPassiveSocket.h"
#include "include/proxyClientActiveSocket.h"
#include "include/proxyOriginActiveSocket.h"

#define MAX_PORTSTRING_SIZE 10

static const fd_handler fd = {
	.handle_read = proxy_client_active_socket_read,
	.handle_write = proxy_client_active_socket_write,
	.handle_block = proxy_client_active_socket_block,
	.handle_close = proxy_client_active_socket_close,
};

fd_handler * proxy_client_active_socket_fd_handler(void){
	return &fd;
}

static int open_origin_socket( char* hostname, unsigned short port );

void proxy_client_active_socket_read(struct selector_key *key){
	proxy_client_active_socket_data * data = (proxy_client_active_socket_data*)(key->data);
	unsigned short port;
	int iport;

	int client_fd = key->fd;
	int origin_fd = data->origin_fd;

	FILE* sockcrfp = fdopen( client_fd, "r" );

	if (origin_fd == -1) {
		char line[10000], method[10000], url[10000], protocol[10000], host[10000], path[10000];

		/* Read the first line of the request. */
		if ( fgets( line, sizeof(line), sockcrfp ) == (char*) 0 )
			send_error( 400, "Bad Request", (char*) 0, "No request found." );

		/* Parse it. */
		trim( line );
		if ( sscanf( line, "%[^ ] %[^ ] %[^ ]", method, url, protocol ) != 3 )
			send_error( 400, "Bad Request", (char*) 0, "Can't parse request." );

		if ( url[0] == '\0' )
			send_error( 400, "Bad Request", (char*) 0, "Null URL." );

		if ( strncasecmp( url, "http://", 7 ) == 0 ) {
			(void) strncpy( url, "http", 4 );                                                                         /* making sure it's lower case */
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
		} else
			send_error( 400, "Bad Request", (char*) 0, "Unknown URL type." );

		/* Open socket to the origin server. */
		int originSocket = open_origin_socket( host, port );

		origin_fd = data->origin_fd = originSocket;

		FILE* sockowfp = fdopen( origin_fd, "w" );

		int ich;
		long content_length, i;

		/* Send request. */
		(void) fprintf( sockowfp, "%s %s %s\r\n", method, path, protocol );
		/* Forward the remainder of the request from the client. */
		content_length = -1;
		while ( fgets( line, sizeof(line), sockcrfp ) != (char*) 0 ) {
			if ( strcmp( line, "\n" ) == 0 || strcmp( line, "\r\n" ) == 0 )
				break;
			(void) fputs( line, sockowfp );
			trim( line );
			if ( strncasecmp( line, "Content-Length:", 15 ) == 0 )
				content_length = atol( &(line[15]) );
		}
		(void) fputs( line, sockowfp );
		(void) fflush( sockowfp );
		/* If there's content, forward that too. */
		if ( content_length != -1 )
			for ( i = 0; i < content_length && ( ich = getchar() ) != EOF; ++i )
				putc( ich, sockowfp );
		(void) fflush( sockowfp );

		proxy_origin_active_socket_data * dataOrigin = malloc(sizeof(proxy_origin_active_socket_data));
		if(SELECTOR_SUCCESS != selector_register(key->s, originSocket, proxy_origin_active_socket_fd_handler(),
		                                         OP_READ, dataOrigin)) {
			DieWithSystemMessage("registering fd failed");
		}

		/* Done. */
		(void) close( client_fd );
		(void) close( origin_fd );
	}
}

static int open_origin_socket( char* hostname, unsigned short port ) {
	int sockfd;
	char portString[MAX_PORTSTRING_SIZE];
	sprintf(portString, "%d", port);

	/* Tell the system what kind(s) of address info we want */
	struct addrinfo addrCriteria;                         // Criteria for address match
	memset(&addrCriteria, 0, sizeof(addrCriteria));         // Zero out structure
	addrCriteria.ai_family = AF_UNSPEC;                   // Any address family
	addrCriteria.ai_socktype = SOCK_STREAM;               // Only stream sockets
	addrCriteria.ai_protocol = IPPROTO_TCP;               // Only TCP protocol

	/* Get address(es) associated with the specified name/service */
	struct addrinfo *addrList;         // Holder for list of addresses returned

	/* Modify servAddr contents to reference linked list of addresses */
	int rtnVal = getaddrinfo(hostname, portString, &addrCriteria, &addrList);
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

		// Connected succesfully!
		break;
	}

	freeaddrinfo(addrList);             // Free addrinfo allocated in getaddrinfo()

	if (connectRet < 0)
		send_error( 503, "Service Unavailable", (char*) 0, "Connection refused." );
	else
		return sockfd;
}

void proxy_client_active_socket_write(struct selector_key *key) {

}

void proxy_client_active_socket_block(struct selector_key *key) {

}

void proxy_client_active_socket_close(struct selector_key *key) {

}
