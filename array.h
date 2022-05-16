#ifndef ARRAY_H
#define ARRAY_H

#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

// Author: Sidhant Puntambekar
// Collaborator: Vignesh Chandrasekhar

#define ARRAY_SIZE 8 // Number of elements in the stack shared array
#define MAX_NAME_LENGTH 256 // Size of each element/string in the stack shared array

// Shared array implementation and interface using a stack. Define struct to hold data pertaining to shared array.
typedef struct {
    char* array[ARRAY_SIZE]; // Storage array for strings (char*)                  
    int counter; // Counter is current position/top of the shared array
    
    int numConsumed; // Tracker for number of items consumed from the shared array
    int numProduced; // Tracker for number of items produced from the shared array

    sem_t empty; // Define a semaphore to track number of empty spaces in the shared array
    sem_t full; // Define a semaphore to track number of full spaces in the shared array
    pthread_mutex_t mutex; // Define a general mutex lock to handle the decrementing/incrementing of the counter variable

} stack;

int array_init(stack *a);                    // init the stack
int array_put(stack *a, char *hostname);     // place element on the top of the stack
int array_get(stack *a, char **hostname);    // remove element from the top of the stack
void array_free(stack *a);                  // free the stack's resources
int arrayPrint(stack *a);                   // Method to print the shared array contents (debugging purposes)

#endif
