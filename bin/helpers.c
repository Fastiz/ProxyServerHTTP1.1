#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "include/helpers.h"
#include "include/proxyClientActiveSocket.h"

#define SERVER_NAME "micro_proxy"
#define SERVER_URL "http://www.acme.com/software/micro_proxy/"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define TIMEOUT 300

void trim(char* line) {
	int l;

	l = strlen( line );
	while ( line[l-1] == '\n' || line[l-1] == '\r' )
		line[--l] = '\0';
}

void DieWithSystemMessage(char * msg) {
	printf("%s", msg);
	exit(1);
}

static void
send_headers(int status, char* title, char* extra_header, char* mime_type, int length, time_t mod, connection_data * connection_data)
{
	time_t now;
	char timebuf[100];

	dprintf(connection_data->client_fd, "%s %d %s\r\n", PROTOCOL, status, title );
	dprintf(connection_data->client_fd, "Server: %s\r\n", SERVER_NAME );
	now = time( (time_t*) 0 );
	strftime( timebuf, sizeof(timebuf), RFC1123FMT, gmtime( &now ) );
	dprintf(connection_data->client_fd, "Date: %s\r\n", timebuf );
	if ( extra_header != (char*) 0 )
		dprintf(connection_data->client_fd, "%s\r\n", extra_header );
	if ( mime_type != (char*) 0 )
		dprintf(connection_data->client_fd, "Content-Type: %s\r\n", mime_type );
	if ( length >= 0 )
		dprintf(connection_data->client_fd, "Content-Length: %d\r\n", length );
	if ( mod != (time_t) -1 )
	{
		strftime( timebuf, sizeof(timebuf), RFC1123FMT, gmtime( &mod ) );
		dprintf(connection_data->client_fd, "Last-Modified: %s\r\n", timebuf );
	}
	dprintf(connection_data->client_fd, "Connection: close\r\n" );
	dprintf(connection_data->client_fd, "\r\n" );
}

void send_error(int status, char* title, char* extra_header, char* text, connection_data * connection_data) {
	send_headers(status, title, extra_header, "text/html", -1, -1, connection_data);
	dprintf(connection_data->client_fd, "\
<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n\
<html>\n\
  <head>\n\
    <meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">\n\
    <title>%d %s</title>\n\
  </head>\n\
  <body bgcolor=\"#cc9999\" text=\"#000000\" link=\"#2020ff\" vlink=\"#4040cc\">\n\
    <h4>%d %s</h4>\n\n",
	       status, title, status, title);
	dprintf(connection_data->client_fd, "%s\n\n", text);
	dprintf(connection_data->client_fd, "\
    <hr>\n\
    <address><a href=\"%s\">%s</a></address>\n\
  </body>\n\
</html>\n",
	       SERVER_URL, SERVER_NAME);
	fflush(stdout);

	kill_client(connection_data);
}

void lineToLowerCase(char* line, int length){
	for(int i=0; i < length; i++) {
		line[i] = tolower(line[i]);
	}
}
