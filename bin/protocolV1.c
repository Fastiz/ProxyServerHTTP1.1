//
// Created by fastiz on 16/06/19.
//

#include <stdio.h>

#include <sys/socket.h>
#include <string.h>


#include "include/configUsers.h"
#include "include/buffer.h"
#include "include/protocolV1.h"


#define INTERNAL_BUFFER_SIZE 1000

void read_structure(struct selector_key * key);
int login(struct selector_key * key);
int metrics(struct selector_key * key);
int transformations(struct selector_key * key);

void returnPermissionDenied(struct selector_key * key){
    protocol_data_v1 * socketData = key->data;
    buffer * write_buffer = socketData->writeBuffer;

    responseHeader resp = {.responseCode=PERMISSION_DENIED};
    buffer_write_data(write_buffer, (void*)&resp, sizeof(responseHeader));
    selector_set_interest_key(key, OP_WRITE);
}

void returnOK(struct selector_key * key){
    protocol_data_v1 * socketData = key->data;
    buffer * write_buffer = socketData->writeBuffer;

    responseHeader resp = {.responseCode=OK};
    buffer_write_data(write_buffer, (void*)&resp, sizeof(responseHeader));
    selector_set_interest_key(key, OP_WRITE);
}

void returnInternalError(struct selector_key * key){
    protocol_data_v1 * socketData = key->data;
    buffer * write_buffer = socketData->writeBuffer;

    responseHeader resp = {.responseCode=INTERNAL_ERROR};
    buffer_write_data(write_buffer, (void*)&resp, sizeof(responseHeader));
    selector_set_interest_key(key, OP_WRITE);
}

int login(struct selector_key * key){
    protocol_data_v1 * socketData = key->data;

    buffer * read_buffer = socketData->readBuffer;
    buffer * write_buffer = socketData->writeBuffer;

    char username[50], password[50];

    int ret1 = buffer_peek_until_null(read_buffer, (char*)username, sizeof(username));
    int ret2 = buffer_peek_until_null(read_buffer, (char*)password, sizeof(password));

    printf("%s, %s\n", username, password);

    if(ret1 > 0 && ret2 > 0){

        buffer_advance_read_to_peek(read_buffer);

        responseHeader resp;


        for(int i=0; i < sizeof(users)/sizeof(userStruct); i++){
            if(strcmp(users[i].username, username) == 0){
                if(strcmp(users[i].password, password) == 0){
                    resp.responseCode = OK;
                    socketData->loggedIn = 1;
                }else{
                    resp.responseCode = PERMISSION_DENIED;
                }

                if(buffer_space(write_buffer)< sizeof(responseHeader))
                    printf("ERROR!\n"); //TODO: error

                buffer_write_data(write_buffer, (void*)&resp, sizeof(responseHeader));
                selector_set_interest_key(key, OP_WRITE);

                socketData->expectedStructureIndex = HEADER;


                return 1;
            }
        }


        returnPermissionDenied(key);

        socketData->expectedStructureIndex = HEADER;

        return 1;
    }

    buffer_reset_peek_line(read_buffer);

    return 0;
}

int metrics(struct selector_key * key){
    protocol_data_v1 * socketData = key->data;
    
    buffer * read_buffer = socketData->readBuffer;

    if(buffer_count(read_buffer) >= sizeof(metricRequest)){
        metricRequest req;
        buffer_read_data(read_buffer, (void*)&req, sizeof(metricRequest));

        //TODO: implementar para cada una de las metricas
        switch (req.metricCode){
            case METRIC1:

                break;
            case METRIC2:

                break;
            default:
                //TODO: error
                break;
        }
        return 1;
    }else{
        return 0;
    }
}

int transformations(struct selector_key * key){
    protocol_data_v1 * socketData = key->data;

    buffer * read_buffer = socketData->readBuffer;
    buffer * write_buffer = socketData->writeBuffer;

    if(buffer_count(read_buffer) >= sizeof(transformationsRequest)){


        transformationsRequest req;
        buffer_peek_data(read_buffer, (void*)&req, sizeof(transformationsRequest));



        //TODO: implementar para cada una de los tipos de transformaciones
        switch (req.type){
            //TODO: por alguna razon cuando se hace login despues de hacer un set_status(0) se rompe todo.
            case SET_STATUS:
                if(buffer_count(read_buffer) >= sizeof(transformationsRequest) + sizeof(setStatusTransformationsRequest)){

                    setStatusTransformationsRequest setReq;

                    buffer_advance_read_to_peek(read_buffer);
                    buffer_read_data(read_buffer, (void*)&setReq, sizeof(setStatusTransformationsRequest));

                    if(socketData->loggedIn){

                        if(setReq.setStatus == 1 || setReq.setStatus == 0){
                            //TODO: llamar a set status
                            printf("setStatus(%d)\n", setReq.setStatus);

                            returnOK(key);
                        }else{
                            returnPermissionDenied(key);
                        }

                    }else{
                        returnPermissionDenied(key);
                    }
                    socketData->expectedStructureIndex=HEADER;
                    return 1;
                }else{
                    buffer_reset_peek_line(read_buffer);
                    return 0;
                }
            case GET_STATUS:
                buffer_advance_read_to_peek(read_buffer);

                if(socketData->loggedIn){
                    //TODO: agarrar el status

                    returnOK(key);
                    getStatusTransformationsResponse res = {.getStatus=1}; //TODO: devolver valor real.

                    buffer_write_data(write_buffer, (void*)&res, sizeof(getStatusTransformationsResponse));
                    selector_set_interest_key(key, OP_WRITE);
                }else{
                    returnPermissionDenied(key);
                }
                socketData->expectedStructureIndex=HEADER;
                return 1;
            case SET_MEDIA_TYPES:
                char media[100];
                int res = buffer_peek_until_null(read_buffer, media, sizeof(media));
                if(res==-1)
                    printf("ERROR\n"); //TODO: error
                if(res>0){
                    buffer_advance_read_to_peek(read_buffer);
                    if(socketData->loggedIn){
                        //TODO: set media types

                        returnOK(key);
                    }else{
                        returnPermissionDenied(key);
                    }
                    socketData->expectedStructureIndex=HEADER;
                    return 1;
                }else{
                    buffer_reset_peek_line(read_buffer);
                    return 0;
                }
            case GET_MEDIA_TYPES:
                buffer_advance_read_to_peek(read_buffer);

                if(socketData->loggedIn){
                    //TODO: agarrar los media types

                    returnOK(key);

                    char* phonyMediaTypes = "hola manola."
                    buffer_write_data(write_buffer, phonyMediaTypes, strlen(phonyMediaTypes)+1);
                    selector_set_interest_key(key, OP_WRITE);

                }else{
                    returnPermissionDenied(key);
                }
                socketData->expectedStructureIndex=HEADER;
                return 1;
            case SET_TRANSFORMATION_COMMAND:
                char command[200];
                int res = buffer_peek_until_null(read_buffer, command, sizeof(command));
                if(res==-1)
                    printf("ERROR\n"); //TODO: error

                if(res>0){
                    buffer_advance_read_to_peek(read_buffer);
                    if(socketData->loggedIn){
                        //TODO: set transformation command

                        returnOK(key);
                    }else{
                        returnPermissionDenied(key);
                    }
                    socketData->expectedStructureIndex=HEADER;
                    return 1;
                }else{
                    buffer_reset_peek_line(read_buffer);
                    return 0;
                }
            case GET_TRANSFORMATION_COMMAND:
                buffer_advance_read_to_peek(read_buffer);

                if(socketData->loggedIn){
                    //TODO: agarrar el comando real

                    returnOK(key);

                    char* phonyTransformationCommand = "como te va."
                    buffer_write_data(write_buffer, phonyTransformationCommand, strlen(phonyTransformationCommand)+1);
                    selector_set_interest_key(key, OP_WRITE);

                }else{
                    returnPermissionDenied(key);
                }
                socketData->expectedStructureIndex=HEADER;
                return 1;
            default:
                //TODO: error
                break;
        }
    }else{
        return 0;
    }
}

void read_structure(struct selector_key * key){
    protocol_data_v1 * socketData = key->data;
    buffer * read_buff = socketData->readBuffer;

    int readSuccess=1;
    while(readSuccess){
        if(socketData->expectedStructureIndex != HEADER){
            switch (socketData->expectedStructureIndex){
                case LOGIN:
                    readSuccess = login(key);
                    break;
                case METRICS:
                    readSuccess = metrics(key);
                    break;
                case TRANSFORMATIONS:
                    readSuccess = transformations(key);
                    break;
                default:
                    //printf("ERROR\n");//TODO: error
                    break;
            }
        }else{

            //Entonces estamos esperando un header
            requestHeader req;
            if(buffer_count(read_buff)>=sizeof(req)){
                buffer_read_data(read_buff, (void*)&req, sizeof(requestHeader));
                if(req.version != 0)
                    //TODO: error
                    ;
                socketData->expectedStructureIndex = req.structureIndex;
            }else{
                readSuccess=0;
            }
        }

    }
 
}

void read_protocol_v1(struct selector_key *key){
    char aux[INTERNAL_BUFFER_SIZE];
    protocol_data_v1 * socketData = key->data;
    buffer * buff = socketData->readBuffer;
    int client_fd = key->fd;

    int ret, to_read;
   // do{
        int available_space = buffer_space(buff);
        to_read = available_space > INTERNAL_BUFFER_SIZE ? INTERNAL_BUFFER_SIZE : available_space;


        ret = recv(client_fd, (void*)aux, to_read, 0);
        if(!ret){
            //TODO: error
            printf("ERROR!\n");
        }
        buffer_write_data(buff, aux, ret);


    //}while(ret == to_read);

    read_structure(key);
}

void write_protocol_v1(struct selector_key *key){
    protocol_data_v1 * socketData = key->data;
    buffer * write_buff = socketData->writeBuffer;

    int to_read;
    while((to_read = buffer_count(write_buff))>0){
        char aux[INTERNAL_BUFFER_SIZE];

        int to_read_max = to_read > INTERNAL_BUFFER_SIZE ? INTERNAL_BUFFER_SIZE : to_read;

        buffer_read_data(write_buff, (char*)&aux, to_read_max);

        int bytes_sent = send(key->fd, &aux, to_read_max, 0);
        if(bytes_sent != to_read_max){
            //TODO: error
            printf("ERROR!\n");
        }
    }

    selector_set_interest_key(key, OP_READ);
}

//TODO: implementar
void close_protocol_v1(struct selector_key *key){
    return;
}


void * protocol_data_init_v1(fd_selector s, int client_fd){
    protocol_data_v1 * data = malloc(sizeof(protocol_data_v1));

    data->loggedIn = 0;
    data->protocol = 0;
    data->expectedStructureIndex = HEADER;
    data->readBuffer = new_buffer();
    data->writeBuffer = new_buffer();

    return data;
}
