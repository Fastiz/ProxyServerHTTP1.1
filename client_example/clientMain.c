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

    printf("usuario: %s, contraseña: %s\n", user, password);


    return bytesSent;
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
    servaddr.sin_port = htons(8080);
    servaddr.sin_addr.s_addr = inet_addr( "127.0.0.1" );

    int ret;

    /* Connect to the server */
    ret = connect( connSock, (struct sockaddr *)&servaddr, sizeof(servaddr) );
    printf("%d\n", ret);

    ret = login("fastiz", "12345", connSock);
    printf("Login: %d\n", ret);

    responseHeader resp;
    recv(connSock, (void*)&resp, sizeof(responseHeader), 0);

    printf("%d\n", resp.responseCode);

    while(1){
        ;
    }

    /* Close our socket and exit */
    close(connSock);

    return 0;
}

