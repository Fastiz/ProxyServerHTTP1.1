#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include "include/selector.h"
#include "include/main.h"
#include "include/proxy.h"

static const int MAXPENDING = 5; // Maximum outstanding connection requests

/* Forwards. */
static int open_client_socket( char* hostname, unsigned short port );
static int open_server_socket( unsigned short port );
static void proxy_http( char* method, char* path, char* protocol, FILE* sockrfp, FILE* sockwfp, FILE* socksrfp, FILE* sockswfp );
static void trim( char* line );
static void send_error( int status, char* title, char* extra_header, char* text ) __attribute__((__noreturn__));
static void DieWithSystemMessage(char * msg);

int main(int argc, char *argv[]) {
	unsigned short port;
	int iport;
	int socketServer;
	selector_status   ss      = SELECTOR_SUCCESS;
	fd_selector selector = NULL;

	//ToDo: checkear que el puerto es entero
	in_port_t servPort = atoi(argv[1]);     // First arg:  local port
	socketServer = open_server_socket(servPort);

	const struct selector_init conf = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec  = 10,
            .tv_nsec = 0,
        },
    };

    if(0 != selector_init(&conf)) {
        DieWithSystemMessage("initializing selector failed");
    }

	selector = selector_new(1024);

    if(selector == NULL) {
		DieWithSystemMessage("unable to create selector");
    }

    const struct fd_handler socksv5 = {
        .handle_read       = proxy_passive_accept,
        .handle_write      = NULL,
        .handle_close      = NULL, // nada que liberar
    };

    ss = selector_register(selector, socketServer, &socksv5,
                                              OP_READ, NULL);
    if(ss != SELECTOR_SUCCESS) {
        DieWithSystemMessage("registering fd failed");
    }

    for(;;) {
        ss = selector_select(selector);
        if(ss != SELECTOR_SUCCESS) {
            DieWithSystemMessage("Runtime error");
        }
    }


	char line[10000], method[10000], url[10000], protocol[10000], host[10000], path[10000];

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

	setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

	// Bind to the local address
	if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
		DieWithSystemMessage("bind() failed");

	// Mark the socket so it will listen for incoming connections
	if (listen(servSock, MAXPENDING) < 0)
		DieWithSystemMessage("listen() failed");

	if(selector_fd_set_nio(servSock) == -1) {
        DieWithSystemMessage("setting server flags failed");
    }

	return servSock;
}