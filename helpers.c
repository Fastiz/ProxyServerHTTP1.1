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
