#ifndef HELPERS_H
#define HELPERS_H

void trim( char* line );
void DieWithSystemMessage(char * msg);
void send_error( int status, char* title, char* extra_header, char* text );
void lineToLowerCase(char* line, int length);

#endif //HELPERS_H
