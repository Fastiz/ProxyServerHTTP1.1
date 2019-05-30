//
// Created by fastiz on 21/05/19.
//

#include "include/main.h"

static const int MAXPENDING = 5; // Maximum outstanding connection requests

/* Forwards. */
static int open_client_socket( char* hostname, unsigned short port );
static int open_server_socket( unsigned short port );
static void proxy_http( char* method, char* path, char* protocol, FILE* sockrfp, FILE* sockwfp, FILE* socksrfp, FILE* sockswfp );
static void trim( char* line );
static void send_error( int status, char* title, char* extra_header, char* text ) __attribute__((__noreturn__));
static void DieWithSystemMessage(char * msg);

int main(int argc, char *argv[]) {
	char line[10000], method[10000], url[10000], protocol[10000], host[10000], path[10000];
	unsigned short port;
	int iport;
	int sockcfd;
	int socksfd;
	FILE* sockcrfp;
	FILE* sockcwfp;
	FILE* socksrfp;
	FILE* sockswfp;

	/*Wait for a client to connect*/
	in_port_t servPort = atoi(argv[1]);     // First arg:  local port
	socksfd = open_server_socket(servPort);

	/* Open separate streams for read and write, r+ doesn't always work. */
	socksrfp = fdopen( socksfd, "r" );
	sockswfp = fdopen( socksfd, "w" );

	/* Read the first line of the request. */
	if ( fgets( line, sizeof(line), socksrfp ) == (char*) 0 )
		send_error( 400, "Bad Request", (char*) 0, "No request found." );

	/* Parse it. */
	trim( line );
	if ( sscanf( line, "%[^ ] %[^ ] %[^ ]", method, url, protocol ) != 3 )
		send_error( 400, "Bad Request", (char*) 0, "Can't parse request." );

	if ( url[0] == '\0' )
		send_error( 400, "Bad Request", (char*) 0, "Null URL." );

	if ( strncasecmp( url, "http://", 7 ) == 0 ) {
		(void) strncpy( url, "http", 4 );                                                         /* making sure it's lower case */
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

	/* Open the client socket to the real web server. */
	sockcfd = open_client_socket( host, port );

	/* Open separate streams for read and write, r+ doesn't always work. */
	sockcrfp = fdopen( sockcfd, "r" );
	sockcwfp = fdopen( sockcfd, "w" );

	proxy_http( method, path, protocol, sockcrfp, sockcwfp, socksrfp, sockswfp );

	/* Done. */
	(void) close( sockcfd );
	(void) close( socksfd );

	exit( 0 );
}


static int open_server_socket(unsigned short port) {
	in_port_t servPort = port;

	// Create socket for incoming connections
	int servSock;                     // Socket descriptor for server
	if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		DieWithSystemMessage("socket() failed");

	// Construct local address structure
	struct sockaddr_in servAddr;                                  // Local address
	memset(&servAddr, 0, sizeof(servAddr));                       // Zero out structure
	servAddr.sin_family = AF_INET;                                // IPv4 address family
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);                 // Any incoming interface
	servAddr.sin_port = htons(servPort);                          // Local port

	// Bind to the local address
	if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
		DieWithSystemMessage("bind() failed");

	// Mark the socket so it will listen for incoming connections
	if (listen(servSock, MAXPENDING) < 0)
		DieWithSystemMessage("listen() failed");

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


static void trim( char* line ) {
	int l;

	l = strlen( line );
	while ( line[l-1] == '\n' || line[l-1] == '\r' )
		line[--l] = '\0';
}

static void DieWithSystemMessage(char * msg) {
	printf("%s", msg);
	exit(1);
}

static void send_error( int status, char* title, char* extra_header, char* text ) {
	printf("%s", title);
	exit( 1 );
}
