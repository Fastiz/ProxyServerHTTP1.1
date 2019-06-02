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

static void
proxy_http( char* method, char* path, char* protocol, FILE* sockcrfp, FILE* sockcwfp, FILE* socksrfp, FILE* sockswfp ) {
	
}