#ifndef HELPERS_H
#define HELPERS_H

#include "connectionData.h"

void trim(char* line);
void printDate();
void printLog(char * str);
void DieWithSystemMessage(char * msg);
void send_error(int status, char* title, char* extra_header, char* text, connection_data * connection_data);
void lineToLowerCase(char* line, int length);

#endif //HELPERS_H
