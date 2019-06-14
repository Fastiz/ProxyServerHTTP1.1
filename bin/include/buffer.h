#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>

#define BUFFER_SIZE 8000

typedef struct buffer {
    char data[BUFFER_SIZE];

    /** reading position */
    int read;

    /** writing position */
    int write;

    /** last peeked line position */
    int peek_line;
} buffer;

buffer * new_buffer();

/* Returns used space in the buffer */
int buffer_count (buffer * buff);

/* Returns available space in the buffer */
int buffer_space (buffer * buff);

int buffer_write_data (buffer * buff, char * data, int size);

int buffer_read_data (buffer * buff, char * dest_buffer, int size);

void buffer_reset_peek_line (buffer * buff);

int buffer_peek_line (buffer * buff, char * dest_buff, int size);

void buffer_free(buffer * buff);

void buffer_clean(buffer * buff);

#endif //BUFFER_H