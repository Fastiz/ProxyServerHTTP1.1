#ifndef HELPERS_H
#define HELPERS_H

void trim( char* line );
void DieWithSystemMessage(char * msg);
void send_error( int status, char* title, char* extra_header, char* text );

#endif //HELPERS_H
