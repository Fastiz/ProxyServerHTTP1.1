#ifndef HELPERS_H
#define HELPERS_H

static void trim( char* line );
static void DieWithSystemMessage(char * msg);
static void send_error( int status, char* title, char* extra_header, char* text );

#endif