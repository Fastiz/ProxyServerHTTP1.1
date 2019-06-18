#include "include/buffer.h"
#include <stdbool.h>
#include <stdio.h>

buffer * new_buffer() {
    buffer * buff = malloc(sizeof(buffer));
    buff->read = 0;
    buff->write = 0;
    buff->peek_line = 0;
    return buff;
}

int buffer_count (buffer * buff) {
    int diff = buff->write - buff->read;
    return diff >= 0 ? diff : BUFFER_SIZE + diff;
}

int buffer_space (buffer * buff) {
    return BUFFER_SIZE - buffer_count(buff) - 1;
}

int buffer_write_data (buffer * buff, char * data, int size) {
    int bytes_to_write = buffer_space(buff) > size ? size : buffer_space(buff);
    for (int i = 0; i < bytes_to_write; i++) {
        buff->data[buff->write] = data[i];
        (buff->write)++;
        if (buff->write == BUFFER_SIZE)
            buff->write = 0;
    }
    return bytes_to_write > 0 ? bytes_to_write : 0;
}

int buffer_read_data (buffer * buff, char * dest_buffer, int size) {
    int bytes_to_read = buffer_count(buff) > size ? size : buffer_count(buff);
    for (int i = 0; i < bytes_to_read; i++) {
        dest_buffer[i] = buff->data[buff->read];
        (buff->read)++;
        if (buff->read == BUFFER_SIZE)
            buff->read = 0;
    }
    return bytes_to_read > 0 ? bytes_to_read : 0;
}

int buffer_peek_data (buffer * buff, char * dest_buffer, int size) {
    int bytes_to_read = buffer_count(buff) > size ? size : buffer_count(buff);
    for (int i = 0; i < bytes_to_read; i++) {
        dest_buffer[i] = buff->data[buff->read];
        (buff->peek_line)++;
        if (buff->peek_line == BUFFER_SIZE)
            buff->peek_line = 0;
    }
    return bytes_to_read > 0 ? bytes_to_read : 0;
}

int buffer_peek_line (buffer * buff, char * dest_buff, int size) {
    int aux = buff->peek_line;

    for (int i = 0; aux != buff->write; i++) {
        if (i >= size)
            return -1;

        dest_buff[i] = buff->data[aux];

        if (buff->data[aux] == '\n') {
            dest_buff[i + 1] = '\0';
            buff->peek_line = (aux + 1 == BUFFER_SIZE) ? 0 : aux + 1;
            return i + 1;
        }

        aux++;
        if (aux == BUFFER_SIZE)
            aux = 0;
    }

    return 0;
}

int buffer_peek_until_null (buffer * buff, char * dest_buff, int size) {
    int aux = buff->peek_line;

    for (int i = 0; aux != buff->write; i++) {
        if (i >= size)
            return -1;

        dest_buff[i] = buff->data[aux];

        if (buff->data[aux] == '\0') {
            dest_buff[i + 1] = '\0';
            buff->peek_line = (aux + 1 == BUFFER_SIZE) ? 0 : aux + 1;
            return i + 1;
        }

        aux++;
        if (aux == BUFFER_SIZE)
            aux = 0;
    }

    return 0;
}

void buffer_advance_read_to_peek(buffer * buff){
    buff->read = buff->peek_line;
}

void buffer_free(buffer * buff) {
    free(buff);
}

void buffer_reset_peek_line(buffer * buff) {
    buff->peek_line = buff->read;
}

void buffer_clean(buffer * buff) {
    buff->write = 0;
    buff->read = 0;
    buff->peek_line = 0;
}