//
// Created by fastiz on 16/06/19.
//

#include <sys/socket.h>

#include "include/buffer.h"
#include "include/protocolV1.h"


#define INTERNAL_BUFFER_SIZE 1000

void read_structure(struct selector_key * key);
int login(struct selector_key * key);
int metrics(struct selector_key * key);
int transformations(struct selector_key * key);

int login(struct selector_key * key){
    data * socketData = key->data;

    structureIndex structure_index = socketData->expectedStructureIndex;

    if(structure_index != LOGIN){
        buffer * write_buffer = socketData->readBuffer;

        int required_length = sizeof(responseHeader);

        if(buffer_space(write_buffer)<required_length){
            //TODO: error
        }

        responseHeader rsp = {(uint8_t)PERMISSION_DENIED};

        buffer_write_data(write_buffer, (void*)&rsp, required_length);
        selector_set_interest_key(key, OP_WRITE);
    }else{

        //TODO: Logica de login
    }

    //TODO: sacar (está por el warning)
    return 1;
}

int metrics(struct selector_key * key){
    data * socketData = key->data;
    
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
    data * socketData = key->data;

    buffer * read_buffer = socketData->readBuffer;

    if(buffer_count(read_buffer) >= sizeof(transformationsRequest)){
        transformationsRequest req;
        buffer_read_data(read_buffer, (void*)&req, sizeof(transformationsRequest));
        
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
                break;
        }
        return 1;
    }else{
        return 0;
    }
}

void read_structure(struct selector_key * key){
    data * socketData = key->data;
    //buffer * read_buff = socketData->readBuffer;

    int readSuccess=1;
    while(readSuccess){
        if(!socketData->loggedIn){
            structureIndex structure_index = socketData->expectedStructureIndex;
            switch (structure_index){
                case METRICS:
                    readSuccess = metrics(key);
                    break;
                case TRANSFORMATIONS:
                    readSuccess = transformations(key);
                    break;
                default:
                    //TODO: error
                    break;
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
    buffer * buff = socketData->readBuffer;
    int client_fd = key->fd;

    int available_space = buffer_space(buff);
    int to_read = available_space > INTERNAL_BUFFER_SIZE ? INTERNAL_BUFFER_SIZE : available_space;


    int ret = recv(client_fd, aux, to_read, 0);
    if(!ret){
        //TODO: error
    }
    buffer_write_data(buff, aux, to_read);
}

void write_protocol_v1(struct selector_key *key){
    data * socketData = key->data;
    buffer * write_buff = socketData->writeBuffer;

    int to_read;
    while((to_read = buffer_count(write_buff))>0){
        char aux[INTERNAL_BUFFER_SIZE];

        int to_read_max = to_read > INTERNAL_BUFFER_SIZE ? INTERNAL_BUFFER_SIZE : to_read;

        buffer_read_data(write_buff, (char*)&aux, to_read_max);

        int bytes_sent = send(key->fd, &aux, to_read_max, 0);
        if(bytes_sent != to_read_max){
            //TODO: error
        }
    }
}
