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

int login(struct selector_key * key){
    protocol_data_v1 * socketData = key->data;

    buffer * read_buffer = socketData->readBuffer;
    buffer * write_buffer = socketData->writeBuffer;

    char username[50], password[50];

    int ret1 = buffer_peek_until_null(read_buffer, (char*)username, sizeof(username));
    int ret2 = buffer_peek_until_null(read_buffer, (char*)password, sizeof(password));

    if(ret1 > 0 && ret2 > 0){
        //TODO: avanzar indice de lectura hasta el indice de peek
        buffer_advance_read_to_peek(read_buffer);

        responseHeader resp;

        printf("Caracteres. Usuarios %d, contraseña: %d.\n", ret1,ret2);
        printf("Strlen. Usuarios %d, contraseña: %d.\n", strlen(username),strlen(password));
        printf("usuario: %s, contraseña: %s\n", username, password);


        for(int i=0; i < sizeof(users)/sizeof(userStruct); i++){
            if(strcmp(users[i].username, username) == 0){
                if(strcmp(users[i].password, password) == 0){
                    printf("Usuario correcto.\n");
                    resp.responseCode = OK;
                    socketData->loggedIn = 1;
                }else{
                    printf("Usuario incorrecto.\n");
                    resp.responseCode = PERMISSION_DENIED;
                }
                buffer_write_data(write_buffer, (void*)&resp, sizeof(responseHeader));
                selector_set_interest_key(key, OP_WRITE);

                socketData->expectedStructureIndex = HEADER;


                return 1;
            }
        }

        printf("No existe el usuario.\n");

        //resp.responseCode = PERMISSION_DENIED;
        resp.responseCode = OK;

        buffer_write_data(write_buffer, (void*)&resp, sizeof(responseHeader));
        selector_set_interest_key(key, OP_WRITE);

        socketData->expectedStructureIndex = HEADER;

        return 1;
    }

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

//TODO: ver si esta loggeado
void read_structure(struct selector_key * key){

    protocol_data_v1 * socketData = key->data;
    buffer * read_buff = socketData->readBuffer;

    int readSuccess=1;
    while(readSuccess){
        if(socketData->expectedStructureIndex != HEADER){
            switch (socketData->expectedStructureIndex){
                case LOGIN:
                    printf("Login\n");
                    readSuccess = login(key);
                    break;
                case METRICS:
                    printf("Metrics\n");
                    //readSuccess = metrics(key);
                    break;
                case TRANSFORMATIONS:
                    printf("Transformations\n");
                    //readSuccess = transformations(key);
                    break;
                default:
                    //TODO: error
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
                printf("Estamos esperando un header %d.\n", req.structureIndex);
            }else{
                readSuccess=0;
            }
        }

    }
 
}

void read_protocol_v1(struct selector_key *key){

    //TODO: acordarse de ver que pasa si el limite es el tamaño de aux
    uint8_t aux[INTERNAL_BUFFER_SIZE];
    protocol_data_v1 * socketData = key->data;
    buffer * buff = socketData->readBuffer;
    int client_fd = key->fd;

    int available_space = buffer_space(buff);
    int to_read = available_space > INTERNAL_BUFFER_SIZE ? INTERNAL_BUFFER_SIZE : available_space;


    int ret = recv(client_fd, aux, to_read, 0);
    if(!ret){
        //TODO: error
    }
    buffer_write_data(buff, aux, to_read);

    aux[to_read+1] = '\0';
    printf("%s\n", aux);

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
        }
    }
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
