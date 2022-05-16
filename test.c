#include "array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

// Author: Sidhant Puntambekar
// Collaborator: Vignesh Chandrasekhar

// Part A (Producers and consumers allowed to do the same amount of work)
#define PRODUCE_THREADS 5 // Define macro for producer threads 
#define CONSUME_THREADS 10 // Define macro for consumer threads
#define NUM_ITEMS 1000 // Number of items to produce
#define CONSUME_ITEMS 500 // Number of items to consume

typedef struct Task {
    stack sharedBuffer;
    pthread_mutex_t countProduced;
    pthread_mutex_t bufferPosition;

} Task; // Thread arguments struct with a reference to the shared array


void* producer (void* args) // Producer function
{
    Task *data = (Task *) args; // Get a reference to our thread args struct that was passed in as an argument
    //char* testString = "Produced"; // String to produce into the shared array by all threads
    char* testStrings[8] = {"aa", "bb", "cc", "dd", "ee", "ff", "gg", "hh"}; // Define set of strings to insert

    for (int i = 0; i < NUM_ITEMS; i++) // Loop through until number of items to produce
    {
        int randIndex = rand() % 8; // Pick a random number between 0 and 7 inclusive
        char* testString = testStrings[randIndex]; // Access that particular string and desinate it to be inserted
        array_put(&(data -> sharedBuffer), testString); // Call array put, thread safety handled by array.c
        // if (array_put(&(data->sharedBuffer), testStrings[i]) == 0)
        // {
        //     printf("%s was produced by thread %lu\n", testStrings[i], pthread_self());
        // }
    }
    return NULL; // Return NULL as default
}


void* consumer(void* args) // Consumer function 
{
    Task *data = (Task *) args; // Get a reference to our thread args struct that was passed in as an argument
    char *hostname = malloc(MAX_NAME_LENGTH); // Malloc a string that is the same length as MAX_NAME_LENGTH for storage
    //char name[MAX_NAME_LENGTH]; //character pointer
    //char *hostname = name;
    //while (data -> num_produced != NUM_ITEMS && data -> sharedBuffer.counter > 0)
    for (int i = 0; i < CONSUME_ITEMS; i++) // Use a for loop to consume items from shared array 
    {
        array_get(&(data -> sharedBuffer), &hostname); // Call array_get. Thread safety handled in array.c
        //data -> num_consumed++;
    }
    free(hostname); // Call free on the malloced hostname to take care of memory leaks
    // while (1)
    // {
    //     pthread_mutex_lock(&(data -> bufferPosition));
    //     pthread_mutex_lock(&(data -> countProduced));

    //     int status1 = data -> sharedBuffer.numProduced == NUM_ITEMS*PRODUCE_THREADS;
    //     int status2 = data -> sharedBuffer.counter <= 0;

    //     if (status1 && status2)
    //     {
    //         //printf("We are done.\n");
    //     pthread_mutex_unlock(&(data -> countProduced));
    //     pthread_mutex_unlock(&(data -> bufferPosition));
    //         return 0;
    //     }
    //     else
    //     {
    //         array_get(&(data -> sharedBuffer), &hostname);
    //     pthread_mutex_unlock(&(data -> countProduced));
    //     pthread_mutex_unlock(&(data -> bufferPosition));
    //     }
    // }
    return NULL; // Return NULL on thread completion
}


int main()
{
    Task shared_array; // Define instance of thread arguments struct 
  
    if (array_init(&shared_array.sharedBuffer) != 0) // Call array_init and check its return value
    {
        printf("Buffer initialization not successful.\n"); // Print error message if unsuccessful
        return -1; // Exit the program with status code -1
    }
    
    pthread_t produce[PRODUCE_THREADS]; // Define producer pool
    pthread_t consume[CONSUME_THREADS]; // Define consumer pool

    // Initialize mutexes (original synchronized check solution for consumers)
    pthread_mutex_init(&(shared_array.countProduced), NULL); 
    pthread_mutex_init(&(shared_array.bufferPosition), NULL);

    for (int i = 0; i < PRODUCE_THREADS; i++) // Create a for loop to create each new producer thread
    {
        if (pthread_create(&produce[i], NULL, producer, &shared_array) != 0) // Call pthread create with an address to the specific thread, producer function to execute and a thread arg struct as parameter
        {
            perror("Failed to create producer thread"); // Print error if it failed
            return -1;
        }
    }

    for (int i = 0; i < CONSUME_THREADS; i++) // Create a for loop to create each new producer thread
    {
        if (pthread_create(&consume[i], NULL, consumer, &shared_array) != 0) // Call pthread create with an address to the specific thread, consumer function to execute and a thread arg struct as parameter
        {
            perror("Failed to create consumer thread"); // Print error if it failed
            return -1;
        }
    }

    for (int i = 0; i < PRODUCE_THREADS; i++) // Create a for loop to join each producer thread
    {
        if (pthread_join(produce[i], NULL) != 0) // Call pthread join on all producer threads such that the caller waits for the thread to finish and clean up resources associated with each thread
        {
            perror("Failed to join producer thread"); // Print error if failed
        }
    }

    for (int i = 0; i < CONSUME_THREADS; i++) // Create a for loop to join each consumer thread
    {
        if (pthread_join(consume[i], NULL) != 0) // Call pthread join on all consumer threads such that the caller waits for the thread to finish and clean up resources associated with each thread
        {
            perror("Failed to join consumer thread"); // Print error if failed
        }
    }

    // Destroy mutexes (original synchronized check solution for consumers)
    pthread_mutex_destroy(&(shared_array.countProduced));
    pthread_mutex_destroy(&(shared_array.bufferPosition));

    // Free the shared array
    array_free(&shared_array.sharedBuffer);

    return 0; // Return 0
}
