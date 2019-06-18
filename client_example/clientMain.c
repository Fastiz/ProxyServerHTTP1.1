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

int login(char * user, char* password, int fd){
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

    return bytesSent == (strlen(user)+1 + strlen(password)+1 + sizeof(requestHeader));
}

int setStatus(int status, int fd){
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

    return bytesSent == sizeof(reqHeader) + sizeof(setStatusTransformationsRequest) + sizeof(transformationsRequest);
}

int main()
{
    int connSock;
    struct sockaddr_in servaddr;

    char buffer[MAX_BUFFER+1];

    /* Create an SCTP TCP-Style Socket */
    connSock = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );

    /* Specify the peer endpoint to which we'll connect */
    bzero( (void *)&servaddr, sizeof(servaddr) );
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MY_PORT_NUM);
    servaddr.sin_addr.s_addr = inet_addr( "127.0.0.1" );

    int ret;
    responseHeader resp;

    /* Connect to the server */
    ret = connect( connSock, (struct sockaddr *)&servaddr, sizeof(servaddr) );
    printf("%d\n", ret);


   /* ret = setStatus(0, connSock);
    recv(connSock, (void*)&resp, sizeof(responseHeader), 0);
    printf("Resultado de setStatus(0): %d\n", resp.responseCode);*/
    ret = login("eze", "12345", connSock);
    recv(connSock, (void*)&resp, sizeof(responseHeader), 0);
    printf("Resultado de login: %d\n", resp.responseCode);

    ret = setStatus(1, connSock);
    recv(connSock, (void*)&resp, sizeof(responseHeader), 0);
    printf("Resultado de setStatus(1): %d\n", resp.responseCode);

    ret = setStatus(0, connSock);
    recv(connSock, (void*)&resp, sizeof(responseHeader), 0);
    printf("Resultado de setStatus(1): %d\n", resp.responseCode);

    ret = login("eze", "12345", connSock);
    recv(connSock, (void*)&resp, sizeof(responseHeader), 0);
    printf("Resultado de login: %d\n", resp.responseCode);


    ret = login("eze", "12345", connSock);
    recv(connSock, (void*)&resp, sizeof(responseHeader), 0);
    printf("Resultado de login: %d\n", resp.responseCode);


    ret = login("eze", "12345", connSock);
    recv(connSock, (void*)&resp, sizeof(responseHeader), 0);
    printf("Resultado de login: %d\n", resp.responseCode);




    ret = login("eze", "12345", connSock);
    recv(connSock, (void*)&resp, sizeof(responseHeader), 0);
    printf("Resultado de login: %d\n", resp.responseCode);

    while(1){
        ;
    }

    /* Close our socket and exit */
    close(connSock);

    return 0;
}

