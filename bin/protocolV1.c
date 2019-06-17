//
// Created by fastiz on 16/06/19.
//

#include <protocolV1.h>
#include <sys/socket.h>

#include "include/buffer.h"
#include "include/protocolV1.h"


#define INTERNAL_BUFFER_SIZE 1000

void read_structure(struct selector_key * key);
int login(struct selector_ket * key);
int metrics(struct selector_ket * key);
int transformations(struct selector_ket * key);

int login(struct selector_ket * key){
    data * socketData = key->data;

    structureIndex structure_index = data->expectedStructure;

    if(structure_index != LOGIN){
        buffer * write_buffer = key->readBuffer

        int required_length = sizeof(responseHeader);

        if(!buffer_space(write_buffer)>=required_length){
            //TODO: error
        }

        responseHeader rsp = {(uint8_t)PERMISSION_DENIED};

        buffer_write_data(write_buffer, rsp, required_length);
        selector_set_interest_key(key, OP_WRITE);
    }else{

        //TODO: Logica de login
    }
}

int metrics(struct selector_ket * key){
    data * socketData = key->data;
    
    buffer * read_buffer = key->readBuffer;

    if(buffer_count(read_buffer) >= sizeof(metricRequest)){
        metricRequest req;
        buffer_read_data(read_buffer, &(void)req, sizeof(metricRequest));

        //TODO: implementar para cada una de las metricas
        switch (req.metricCode){
            case METRIC1:

                break;
            case METRIC2:

                break;
            default:
                //TODO: error
        }
        return 1;
    }else{
        return 0;
    }
}

int transformations(struct selector_ket * key){
    data * socketData = key->data;

    buffer * read_buffer = key->readBuffer;

    if(buffer_count(read_buffer) >= sizeof(transformationsRequest)){
        transformationsRequest req;
        buffer_read_data(read_buffer, &(void)req, sizeof(transformationsRequest));

        typedef enum {SET_STATUS=0, GET_STATUS, SET_MEDIA_TYPES, GET_MEDIA_TYPES, SET_TRANSFORMATION_COMMAND, GET_TRANSFORMATION_COMMAND} transformationsRequestTypes;

        //TODO: implementar para cada una de los tipos de transformaciones
        switch (req.type){
            case SET_STATUS:

                break;
            case GET_STATUS:

                break;
            case SET_MEDIA_TYPES:

                break;
            case GET_MEDIA_TYPES:

                break;
            case SET_TRANSFORMATION_COMMAND:

                break;
            case GET_TRANSFORMATION_COMMAND:

                break;
            default:
                //TODO: error
        }
        return 1;
    }else{
        return 0;
    }
}

void read_structure(struct selector_key * key){
    data * socketData = key->data;
    buffer * read_buff = data->readBuffer;

    int readSuccess=1;
    while(readSuccess){
        if(!data->loggedIn){
            structureIndex structure_index = data->expectedStructure;
            switch (data->expectedStructure){
                case METRICS:
                    readSuccess = metrics(key);
                    break;
                case TRANSFORMATIONS:
                    readSuccess = transformations(key);
                    break;
                default:
                    //TODO: error
            }
        }else{
            readSuccess = login(key);
        }

    }
 
}

void read_protocol_v1(struct selector_key *key){
    //TODO: acordarse de ver que pasa si el limite es el tamaño de aux
    char aux[INTERNAL_BUFFER_SIZE];
    data * socketData = key->data;
    buffer * buff = data->readBuffer
    int client_fd = key->fd;

    int available_space = buffer_space(buff);
    int to_read = available_space > INTERNAL_BUFFER_SIZE ? aux : available_space;


    int ret = recv(client_fd, aux, to_read, 0);
    if(!ret){
        //TODO: error
    }
    buffer_write_data(buff, aux, to_read);
}

void write_protocol_v1(struct selector_key *key){
    data * socketData = key->data;
    buffer * write_buff = data->writeBuffer;

    int to_read;
    while(to_read = buffer_count(write_buff)){
        char aux[INTERNAL_BUFFER_SIZE];

        int to_read_max = to_read > INTERNAL_BUFFER_SIZE ? INTERNAL_BUFFER_SIZE : to_read;

        buffer_read_data(write_buff, &aux, to_read_max);

        int bytes_sent = send(data->fd, &aux, to_read_max, 0);
        if(bytes_sent != to_read_max){
            //TODO: error
        }
    }
}

void close_protocol_v1(struct selector_key *key){
    //TODO: que deberia pasar cuando se cierra el protocolo?
}