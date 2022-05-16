#include "array.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Author: Sidhant Puntambekar
// Collaborator: Vignesh Chandrasekhar

int array_init(stack *a) // Init the stack
{
    int initIndex = 0; // Define a counter variable for shared array initialization

    while (initIndex < ARRAY_SIZE) // Use a while loop to traverse through shared array
    { 
        a -> array[initIndex] = (char*) malloc(MAX_NAME_LENGTH); // Malloc a sufficient length of memory on the heap to store each string (case to char*)
        initIndex = initIndex + 1; // Increment the init counter variable 
        //printf("Initing array: %d\n", initIndex); // Print that we are initializing the shared array at each particular index
    }
    
    a -> counter = 0; // Initialize stack position counter
    a -> numProduced = 0; // Initialize the number of items produced counter
    a -> numConsumed = 0; // Initialize the number of items consumed counter

    // Initialize semaphores
    // Initialize the stack counter mutex lock
    // Initialize the empty semaphore with an initial value of ARRAY_SIZE
    // Initialize the full sempahore with an initial value of 0
    if (pthread_mutex_init(&(a -> mutex), NULL) != 0 || sem_init(&(a -> empty), 0, ARRAY_SIZE) != 0 || sem_init(&(a -> full), 0, 0) != 0)
    { 
        printf("Initialization of semaphores and mutex lock failed.\n");
        return -1;    
    }
    
    return 0; // Return 0 indicating success from the initialization
}

int array_put(stack *a, char *hostname) // Place hostname string on top of the stack. Block when array is full
{
    //printf("In put function\n");
    // Bounded buffer solution
    sem_wait(&(a -> empty)); // Call sem_wait on the empty semaphore (decrements number of empty spaces by 1). Producer wait/sleep when there are no empty slots (all slots are filled)
    pthread_mutex_lock(&(a -> mutex)); // Lock the stack counter mutex

    // int semValue = 0; 
    // sem_getvalue(&(a -> empty), &semValue); 
    // printf("The initial value of the semaphore is %d\n", semValue);
    
    // Critical section for array_put
    if (strlen(hostname) + 1 > MAX_NAME_LENGTH) // If the string length of hostname is greater than the MAX_NAME_LENGTH macro
    {
       printf("Exceeded max hostname length for shared array.\n"); // Print that we exceeded the max hostname length
       return -1; // Return -1
    }

    // hostname is being produced into the next available position in shared array by the producer
    if (strncpy(a -> array[a -> counter], hostname, MAX_NAME_LENGTH) == NULL) // Call strncpy to copy hostname to next available index in the stack
    {
        printf("Unable to put hostname into stack.\n"); // If it failed, print an error message
        return -1; // Return -1
    }
    a -> counter = a -> counter + 1; // Increase stack counter by 1
    a -> numProduced = a -> numProduced + 1; // Increase number of produced items by 1

    // printf("Put counter: %d\n", a -> counter); // Print where the counter is
    // printf("Number of items produced: %d\n", a -> numProduced); // Print the number of items produced
    // printf("Capacity: %d / 8\n", a -> counter); // Print the capacity of the shared array
    // printf("Put: %s\n", a -> array[a -> counter - 1]); // Show the current string put into the shared array
    // printf("\n"); // Print new line

    pthread_mutex_unlock(&(a -> mutex)); // Unlock the mutex
    sem_post(&(a -> full)); // Post to full/wake up consumer saying that a slot has been filled (can start consuming again)
    
    return 0; // Return 0 on success
}

int array_get(stack *a, char** hostname) // Remove element from the top of stack. Block when array is empty
{
    sem_wait(&(a -> full)); // Call sem_wait on the full semaphore (decrements number of full spaces by 1). Consumer wait/sleep when there are no full slots (all slots are empty)
    pthread_mutex_lock(&(a -> mutex)); // Lock the shared mutex for counter

    // if (a -> counter == 0)
    // {
    //     pthread_mutex_unlock(&(a -> mutex));
    //     sem_post(&(a -> full));
    //     pthread_exit(0);
    // }

    // Critical section for array_get
    a -> counter = a -> counter - 1; // Decrement the counter by one to remove element
    if (strncpy(*hostname, a -> array[a -> counter], MAX_NAME_LENGTH) == NULL) // Use strcpy again to copy to dereferenced hostname addressed
    {
        printf("Failed to copy"); // If strcpy failed, return an error message
        return -1; // Return -1
    }
    //*hostname = a -> array[a -> counter];
    
    //a -> array[a -> counter - 1] = temp;
    a -> numConsumed = a -> numConsumed + 1; // Increment the number of items consumed by 1

    // printf("Get counter: %d\n", a -> counter); // Print the current position of the counter
    // printf("Number of items consumed: %d\n", a -> numConsumed); // Print the number of items consumed
    // printf("Capacity: %d / 8\n", a -> counter); // Print the capacity of the shared array
    // printf("Got: %s\n", *hostname); // Print the got hostname
    // printf("\n"); // Print new line

    pthread_mutex_unlock(&(a -> mutex)); // Unlock the mutex
    sem_post(&(a -> empty)); // Post/wake up producer that we have a new empty slot (a slot was freed).
    return 0; // Return 0 indicating success 
}

void array_free(stack *a) // Free the stack's resources
{
    // Free/destroy semaphores and mutex lock
    pthread_mutex_destroy(&(a -> mutex));
    sem_destroy(&(a -> full));
    sem_destroy(&(a -> empty));

    int freeIndex = 0; // Designate index to free shared array
    while (freeIndex < ARRAY_SIZE) // While loop to traverse each shared array element
    { 
        free(a -> array[freeIndex]); // Call free on each index
        freeIndex = freeIndex + 1; // Increment the free counter by 1
    }
}

int arrayPrint(stack* a) // Print each stack element
{
    for (int i = 0; i < a -> counter; i++) // Use a for loop
    {
        printf("%s", (a -> array[i])); // Log each element
    }
    printf("\n");
    return 0;
}
