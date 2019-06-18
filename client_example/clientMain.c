//
// Created by fastiz on 17/06/19.
//

#include "clientMain.h"


/*
 *  sctpclnt.c
 *
 *  SCTP multi-stream client.
 *
 *  M. Tim Jones <mtj@mtjones.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#include <string.h>

#define MAX_BUFFER 1000
#define MY_PORT_NUM 9090

//Includes of structures v1
#include "../bin/include/protocolV1.h"

void login(char * user, char* password, int fd){
    int bytesSent = 0;

    requestHeader reqHeader = {
            .structureIndex = LOGIN,
            .version = 1
    };

    //Send header
    bytesSent += send(fd, (void*)&reqHeader, sizeof(reqHeader), 0);

    //Send user and password
    bytesSent += send(fd, user, strlen(user)+1, 0);
    bytesSent += send(fd, password, strlen(password)+1, 0);

    responseHeader resp;
    recv(fd, (void*)&resp, sizeof(responseHeader), 0);

    if(resp.responseCode == OK){
        printf("Logged in successfully.\n");
    }else if(resp.responseCode == PERMISSION_DENIED){
        printf("Invalid username of password.\n");
    }else{
        printf("An error ocurred when logging in.\n");
    }
}

void setStatus(int status, int fd){
    int bytesSent = 0;

    requestHeader reqHeader = {
            .structureIndex = TRANSFORMATIONS,
            .version = 1
    };

    //Send header
    bytesSent += send(fd, (void*)&reqHeader, sizeof(reqHeader), 0);

    transformationsRequest transf = {.type=SET_STATUS};
    //Send transformation request
    bytesSent += send(fd, (void*)&transf, sizeof(transformationsRequest), 0);


    setStatusTransformationsRequest statusReq = {.setStatus=(uint8_t)status};
    //send value
    bytesSent += send(fd, (void*)&statusReq, sizeof(setStatusTransformationsRequest), 0);

    responseHeader resp;
    recv(fd, (void*)&resp, sizeof(responseHeader), 0);

    if(resp.responseCode == OK){
        printf("Transformations status was correctly set to %d.\n", status);
    }else if(resp.responseCode == PERMISSION_DENIED){
        printf("It is required to login first.\n");
    }else{
        printf("An error ocurred.\n");
    }
}

void getStatus(int fd){
    int bytesSent = 0;

    requestHeader reqHeader = {
            .structureIndex = TRANSFORMATIONS,
            .version = 1
    };

    //Send header
    bytesSent += send(fd, (void*)&reqHeader, sizeof(reqHeader), 0);

    transformationsRequest transf = {.type=GET_STATUS};
    //Send transformation request
    bytesSent += send(fd, (void*)&transf, sizeof(transformationsRequest), 0);


    responseHeader resp;
    recv(fd, (void*)&resp, sizeof(responseHeader), 0);


    if(resp.responseCode == OK){
        getStatusTransformationsResponse getStatusResponse;
        recv(fd, (void*)&getStatusResponse, sizeof(getStatusTransformationsResponse),0);
        printf("Transformations status is %d.\n", getStatusResponse.getStatus);
    }else if(resp.responseCode == PERMISSION_DENIED){
        printf("It is required to login first.\n");
    }else{
        printf("An error ocurred.\n");
    }

}

void setMediaTypes(char * mediaTypes, int fd){
    int bytesSent = 0;

    requestHeader reqHeader = {
            .structureIndex = TRANSFORMATIONS,
            .version = 1
    };

    //Send header
    bytesSent += send(fd, (void*)&reqHeader, sizeof(reqHeader), 0);

    transformationsRequest transf = {.type=SET_MEDIA_TYPES};
    //Send transformation request
    bytesSent += send(fd, (void*)&transf, sizeof(transformationsRequest), 0);

    bytesSent += send(fd, (void*)mediaTypes, strlen(mediaTypes)+1, 0);

    responseHeader resp;
    recv(fd, (void*)&resp, sizeof(responseHeader), 0);

    if(resp.responseCode == OK){
        printf("Transformation media types set to '%s'.\n", mediaTypes);
    }else if(resp.responseCode == PERMISSION_DENIED){
        printf("It is required to login first.\n");
    }else{
        printf("An error ocurred.\n");
    }
}


void getMediaTypes(int fd){
    int bytesSent = 0;

    requestHeader reqHeader = {
            .structureIndex = TRANSFORMATIONS,
            .version = 1
    };

    //Send header
    bytesSent += send(fd, (void*)&reqHeader, sizeof(reqHeader), 0);

    transformationsRequest transf = {.type=GET_MEDIA_TYPES};
    //Send transformation request
    bytesSent += send(fd, (void*)&transf, sizeof(transformationsRequest), 0);



    responseHeader resp;
    recv(fd, (void*)&resp, sizeof(responseHeader), 0);



    if(resp.responseCode == OK){
        char mediaTypes[200];

        int i=0;
        char c;
        while((recv(fd, &c, 1, 0)>0 && c!='\0' && i < sizeof(mediaTypes))){
            mediaTypes[i] = c;
            i++;
        }
        mediaTypes[i]='\0';
        printf("Transformation media types set to '%s'.\n", mediaTypes);
    }else if(resp.responseCode == PERMISSION_DENIED){
        printf("It is required to login first.\n");
    }else{
        printf("An error ocurred.\n");
    }
}

void setTransformationCommand(char * command, int fd){
    int bytesSent = 0;

    requestHeader reqHeader = {
            .structureIndex = TRANSFORMATIONS,
            .version = 1
    };

    //Send header
    bytesSent += send(fd, (void*)&reqHeader, sizeof(reqHeader), 0);

    transformationsRequest transf = {.type=SET_TRANSFORMATION_COMMAND};
    //Send transformation request
    bytesSent += send(fd, (void*)&transf, sizeof(transformationsRequest), 0);

    bytesSent += send(fd, (void*)command, strlen(command)+1, 0);

    responseHeader resp;
    recv(fd, (void*)&resp, sizeof(responseHeader), 0);

    if(resp.responseCode == OK){
        printf("Transformation command set to '%s'.\n", command);
    }else if(resp.responseCode == PERMISSION_DENIED){
        printf("It is required to login first.\n");
    }else{
        printf("An error ocurred.\n");
    }
}

void getTransformationCommand(int fd){
    int bytesSent = 0;

    requestHeader reqHeader = {
            .structureIndex = TRANSFORMATIONS,
            .version = 1
    };

    //Send header
    bytesSent += send(fd, (void*)&reqHeader, sizeof(reqHeader), 0);

    transformationsRequest transf = {.type=GET_TRANSFORMATION_COMMAND};
    //Send transformation request
    bytesSent += send(fd, (void*)&transf, sizeof(transformationsRequest), 0);

    responseHeader resp;
    recv(fd, (void*)&resp, sizeof(responseHeader), 0);



    if(resp.responseCode == OK){

        char command[200];

        int i=0;
        char c;
        while((recv(fd, &c, 1, 0)>0 && c!='\0' && i < sizeof(command))){
            command[i] = c;
            i++;
        }
        command[i]='\0';

        printf("Transformation command set to '%s'.\n", command);
    }else if(resp.responseCode == PERMISSION_DENIED){
        printf("It is required to login first.\n");
    }else{
        printf("An error ocurred.\n");
    }
}

void getMetric(int metric, int fd){
    int bytesSent = 0;

    requestHeader reqHeader = {
            .structureIndex = METRICS,
            .version = 1
    };

    //Send header
    bytesSent += send(fd, (void*)&reqHeader, sizeof(reqHeader), 0);

    metricRequest mreq = {.metricCode=metric};
    //Send transformation request
    bytesSent += send(fd, (void*)&mreq, sizeof(mreq), 0);

    responseHeader resp;
    recv(fd, (void*)&resp, sizeof(responseHeader), 0);

    if(resp.responseCode == OK){

        metricResponse mresp;
        recv(fd, (void*)&mresp, sizeof(metricResponse),0);

        printf("Metric response: '%d'.\n", mresp.response);

    }else if(resp.responseCode == PERMISSION_DENIED){
        printf("It is required to login first.\n");
    }else{
        printf("An error ocurred.\n");
    }
}

void findCommand(char * line, int fd){
    char username[50], password[50], mediaTypes[1000], command[1000];
    int num;
    if(sscanf(line, "login %s %s", username, password)==2){
        login(username, password, fd);
    }else if(sscanf(line, "setStatus %d", &num)==1){
        setStatus(num, fd);
    }else if(strcmp(line, "getStatus\n")==0) {
        getStatus(fd);
    }else if(sscanf(line, "setMediaTypes %s", mediaTypes)==1) {
        setMediaTypes(mediaTypes, fd);
    }else if(strcmp(line, "getMediaTypes\n")==0) {
        getMediaTypes(fd);
    }else if(sscanf(line, "setTransformationCommand %[^\n]s", command)==1){
        setTransformationCommand(command, fd);
    }else if(strcmp(line, "getTransformationCommand\n")==0){
        getTransformationCommand(fd);
    }else if(strcmp(line, "getCurrentConnections\n")==0){
        getMetric(ACTUAL_CONNECTIONS,fd);
    }else if(strcmp(line, "getTotalConnections\n")==0){
        getMetric(ALL_TIME_CONNECTIONS,fd);
    }else if(strcmp(line, "getBytesTransferred\n")==0){
        getMetric(BYTES_TRANSFERRED,fd);
    }else{
        printf("Invalid command.\n");
    }
}


int main()
{
    int connSock;
    struct sockaddr_in servaddr;

    /* Create an SCTP TCP-Style Socket */
    connSock = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );

    /* Specify the peer endpoint to which we'll connect */
    bzero( (void *)&servaddr, sizeof(servaddr) );
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MY_PORT_NUM);
    servaddr.sin_addr.s_addr = inet_addr( "127.0.0.1" );

    int ret;

    /* Connect to the server */
    ret = connect( connSock, (struct sockaddr *)&servaddr, sizeof(servaddr) );
    if(ret==0){
        printf("Connected successfully to proxy server.\n");
    }else{
        printf("Could not connect to proxy server.\n");
        return 1;
    }

    char * line;
    long unsigned int len;
    while(1){
        getline(&line, &len, stdin);

        findCommand(line, connSock);
        free(line);
    }

    /* Close our socket and exit */
    close(connSock);

    return 0;
}

