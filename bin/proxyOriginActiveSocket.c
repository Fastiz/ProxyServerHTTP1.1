#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "include/proxyOriginActiveSocket.h"
#include "include/helpers.h"

static int getLine(char * lineBuff, int size, int fd);

static const fd_handler fd = {
	.handle_read = proxy_origin_active_socket_read,
	.handle_write = proxy_origin_active_socket_write,
	.handle_block = proxy_origin_active_socket_block,
	.handle_close = proxy_origin_active_socket_close,
};

fd_handler * proxy_origin_active_socket_fd_handler(void){
	return &fd;
}

void proxy_origin_active_socket_write(struct selector_key *key){
	/*char buff[MAX_BUFF_SIZE];
	   int bytesRead = recv(key->data->client_fd, buff, MAX_BUFF_SIZE, 0);
	   if(bytesRead == -1)
	    DieWithSystemMessage("Error when reading client socket.");

	   int result = send(key->fd, buff, bytesRead, 0);
	   if(result == -1){
	    if(errno == EWOULDBLOCK){
	        //key->data->
	    }else{
	        DieWithSystemMessage("Error when writing origin socket.")
	    }
	   }*/
}

void proxy_origin_active_socket_read(struct selector_key *key) {
	char line[10000], protocol2[10000], comment[10000];
	proxy_origin_active_socket_data * data = (proxy_origin_active_socket_data*)(key->data);

	int origin_fd = key->fd;
	int client_fd = data->client_fd;

	//FILE* sockorfp = fdopen( origin_fd, "r" );
	//FILE* sockcwfp = fdopen( client_fd, "w" );

	/* Forward the response back to the client. */
	int content_length = -1;
	int first_line = 1;
	int status = -1;

	while ( getLine(line, sizeof(line), origin_fd) > 0 ) {
		if ( strcmp( line, "\n" ) == 0 || strcmp( line, "\r\n" ) == 0 )
			break;

		send(client_fd, line, strlen(line), 0);

		trim( line );
		if ( first_line ) {
			(void) sscanf( line, "%[^ ] %d %s", protocol2, &status, comment );
			first_line = 0;
		}
		if ( strncasecmp( line, "Content-Length:", 15 ) == 0 )
			content_length = atol( &(line[15]) );
	}

	/* Add a response header. */
	char * str = "Connection: close\r\n";
	send(client_fd, str, strlen(str), 0);
	send(client_fd, line, strlen(line), 0);

	/* Under certain circumstances we don't look for the contents, even
	** if there was a Content-Length.
	*/

    int i, ich;

	//if ( strcasecmp( method, "HEAD" ) != 0 && status != 304 ) {
	/* Forward the content too, either counted or until EOF. */
	for ( i = 0; ( content_length == -1 || i < content_length ) && ( recv(origin_fd, &ich, 1, 0) > 0 ); ++i ) {
		send(client_fd, &ich, 1, 0);
	}

	//}

	/* Done. */
	//(void) close( client_fd );
	//(void) close( origin_fd );
}

static int getLine(char * lineBuff, int size, int fd) {
	char c = 0;
	int count = 0;
	int ret = recv(fd, &c, 1, 0);

	while (c != '\n') {
		lineBuff[count] = c;
		count += ret;
		ret = recv(fd, &c, 1, 0);
	}

	lineBuff[count] = '\n';
	lineBuff[count + 1] = 0;

	return count;
}

void proxy_origin_active_socket_block(struct selector_key *key) {

}

void proxy_origin_active_socket_close(struct selector_key *key) {

}
