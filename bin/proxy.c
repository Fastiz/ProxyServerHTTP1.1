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
#include "selector.h"

#define MAX_PORTSTRING_SIZE 10

void proxy_passive_accept(struct selector_key *key) {
	for (;;) {                 // Run forever
		struct sockaddr_in clntAddr;                                 // Client address
		// Set length of client address structure (in-out parameter)
		socklen_t clntAddrLen = sizeof(clntAddr);

		// Wait for a client to connect
		int clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
		if (clntSock < 0)
			DieWithSystemMessage("accept() failed");

		// clntSock is connected to a client!

		char clntName[INET_ADDRSTRLEN];                                 // String to contain client address
		if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName,
		              sizeof(clntName)) != NULL)
			printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
		else
			puts("Unable to get client address");

		//HandleTCPClient(clntSock);
		return clntSock;
	}
	// NOT REACHED
}

static int open_client_socket( char* hostname, unsigned short port ) {
	int sockfd;
	char portString[MAX_PORTSTRING_SIZE];
	sprintf(portString, "%d", port);

	// Tell the system what kind(s) of address info we want
	struct addrinfo addrCriteria;                 // Criteria for address match
	memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
	addrCriteria.ai_family = AF_UNSPEC;           // Any address family
	addrCriteria.ai_socktype = SOCK_STREAM;       // Only stream sockets
	addrCriteria.ai_protocol = IPPROTO_TCP;       // Only TCP protocol

	// Get address(es) associated with the specified name/service
	struct addrinfo *addrList; // Holder for list of addresses returned
	// Modify servAddr contents to reference linked list of addresses
	int rtnVal = getaddrinfo(hostname, portString, &addrCriteria, &addrList);
	if (rtnVal != 0)
		send_error( 404, "Not Found", (char*) 0, "Unknown host." );

	int connectRet;
	// Iterate over returned addresses
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

	freeaddrinfo(addrList);     // Free addrinfo allocated in getaddrinfo()

	if (connectRet < 0)
		send_error( 503, "Service Unavailable", (char*) 0, "Connection refused." );
	else
		return sockfd;
}

static void
proxy_http( char* method, char* path, char* protocol, FILE* sockcrfp, FILE* sockcwfp, FILE* socksrfp, FILE* sockswfp ) {
	char line[10000], protocol2[10000], comment[10000];
	int first_line, status, ich;
	long content_length, i;

	/* Send request. */
	(void) fprintf( sockcwfp, "%s %s %s\r\n", method, path, protocol );
	/* Forward the remainder of the request from the client. */
	content_length = -1;
	while ( fgets( line, sizeof(line), socksrfp ) != (char*) 0 ) {
		if ( strcmp( line, "\n" ) == 0 || strcmp( line, "\r\n" ) == 0 )
			break;
		(void) fputs( line, sockcwfp );
		trim( line );
		if ( strncasecmp( line, "Content-Length:", 15 ) == 0 )
			content_length = atol( &(line[15]) );
	}
	(void) fputs( line, sockcwfp );
	(void) fflush( sockcwfp );
	/* If there's content, forward that too. */
	if ( content_length != -1 )
		for ( i = 0; i < content_length && ( ich = getchar() ) != EOF; ++i )
			putc( ich, sockcwfp );
	(void) fflush( sockcwfp );

	/* Forward the response back to the client. */
	content_length = -1;
	first_line = 1;
	status = -1;
	while ( fgets( line, sizeof(line), sockcrfp ) != (char*) 0 ) {
		if ( strcmp( line, "\n" ) == 0 || strcmp( line, "\r\n" ) == 0 )
			break;

		(void) fputs( line, sockswfp );
		trim( line );
		if ( first_line ) {
			(void) sscanf( line, "%[^ ] %d %s", protocol2, &status, comment );
			first_line = 0;
		}
		if ( strncasecmp( line, "Content-Length:", 15 ) == 0 )
			content_length = atol( &(line[15]) );
	}
	/* Add a response header. */
	(void) fputs( "Connection: close\r\n", sockswfp );
	(void) fputs( line, sockswfp );
	(void) fflush( sockswfp );
	/* Under certain circumstances we don't look for the contents, even
	** if there was a Content-Length.
	*/
	if ( strcasecmp( method, "HEAD" ) != 0 && status != 304 ) {
		/* Forward the content too, either counted or until EOF. */
		for ( i = 0; ( content_length == -1 || i < content_length ) && ( ich = getc( sockcrfp ) ) != EOF; ++i ) {
			putc( ich, sockswfp );
		}
	}
	(void) fflush( sockswfp );
}