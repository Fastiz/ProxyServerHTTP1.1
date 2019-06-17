

#ifndef CONFIGUSERS_H
#define CONFIGUSERS_H

typedef struct {
    char username[50];
    char password[50];
} userStruct;

static userStruct users[2] = {
        { .username = "fastiz", .password = "12345"},
        { .username = "eze", .password="12345"}
};


#endif