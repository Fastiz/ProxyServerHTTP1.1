//
// Created by fastiz on 16/06/19.
//

#ifndef PROXYSTRUCTURESV1_H
#define PROXYSTRUCTURESV1_H

#include <stdint.h>
#include "selector.h"

void read_protocol_v1(struct selector_key *key);
void write_protocol_v1(struct selector_key *key);



//TODO: cambiar las metricas aca y en el .c
typedef enum {METRIC1, METRIC2, METRIC3, METRIC4} metricsEnum;

typedef enum {LOGIN=0, METRICS, TRANSFORMATIONS} structureIndex;

typedef enum {PERMISSION_DENIED=-2, INTERNAL_ERROR, OK} responseNumber;

typedef enum {SET_STATUS=0, GET_STATUS, SET_MEDIA_TYPES, GET_MEDIA_TYPES, SET_TRANSFORMATION_COMMAND, GET_TRANSFORMATION_COMMAND} transformationsRequestTypes;

/**
 *  DATA
 */
typedef struct {
    uint8_t loggedIn;
    uint8_t protocol;
    uint8_t expectedStructureIndex;
    buffer * readBuffer;
    buffer * writeBuffer;
} data;


/**
 * REQUESTS
 */

typedef struct {
    uint8_t version;
    uint8_t structureIndex;
} requestHeader;

typedef struct {
    char * username;
    char * password;
} loginRequest;

typedef struct {
    uint8_t metricCode;
} metricRequest;

typedef struct {
    uint8_t type;
}transformationsRequest;

typedef struct {
    uint8_t setStatus;
} setStatusTransformationsRequest;

//argument ends with \0
typedef struct {
    char* argument;
} setTransformationsRequest;


/**
 * RESPONSES
 */
typedef struct {
    uint8_t responseCode;
} responseHeader;

typedef struct {
    uint32_t size;
} metricResponse;

typedef struct {
    uint8_t getStatus;
} getStatusTransformationsResponse;

//argument ends with \0
typedef struct {
    char* argument;
} defaultTransformationResponse;


#endif
