#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include "array.h"
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define ARRAY_SIZE 8
#define MAX_INPUT_FILES 100
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10
#define MAX_NAME_LENGTH 256
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

typedef struct {
    stack shared_array; // Define thread safe stack shared array

    FILE* serviced; // File pointer for serviced hostnames
    FILE* results; // File pointer for resolved hostnames with IPs

    int servicedFileCounter; // Counter to track which files have been serviced. Shared among requesters
    int numFilesToService; // Track the number of files to service in total 
    int producersLeft; // Tracks the number of producers left

    char* servicedFileArray[MAX_INPUT_FILES]; // File names to service from array

    // Lock for producers, consumers, lock for anything shared
    pthread_mutex_t stdErrLock; // Mutex lock for any printing to stderr
    pthread_mutex_t stdOutLock; // Mutex lock for any printing to stdout
    pthread_mutex_t producersLeftLock; // Mutex lock for shared variable numProducers
    pthread_mutex_t servicedFileCounterLock; // Mutex lock for servicedFileCounter
    pthread_mutex_t servicedWriteLock; // Serviced.txt write file lock
    pthread_mutex_t resultsWriteLock; // Results.txt write file lock

} Buffer;

void* requester(void* args); // Requester function header
void* resolver(void* args); // Resolver function header

#endif
