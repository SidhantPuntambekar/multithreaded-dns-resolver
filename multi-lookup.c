#include "multi-lookup.h" // Multilookup header
#include "array.h" // Shared array header
#include "util.h" // DNS Lookup
#include <sys/time.h> // Timing the main function

// Author: Sidhant Puntambekar

void* requester(void* args) // Requesters should read hostnames from the file and then insert into shared array
{
    Buffer* data = (Buffer*) args; // Cast void pointer to a Buffer pointer (to be used when accessing struct data)
    stack* a = &(data -> shared_array); // Define reference to shared array for ease of use with array_put

    int numHostnameFilesServiced = 0; // Tracks the number of hostname files serviced by each thread

    while (1) // Begin an infinite while loop (so producers can only terminate in one way)
    {
        pthread_mutex_lock(&(data -> servicedFileCounterLock)); // Lock the servicedFileCounter lock
        if (data -> servicedFileCounter >= data -> numFilesToService) // Check if each thread's serviced file counter has gone over the limit of number of files to service. If true, we can exit as a producer
        {
            pthread_mutex_unlock(&(data -> servicedFileCounterLock)); // Unlock the servicedFileCounter lock

            // pthread_mutex_lock(&(data -> producersLeftLock));
            // data -> producersLeft = data -> producersLeft - 1;
            // pthread_mutex_unlock(&(data -> producersLeftLock));

            printf("thread %lx serviced %d files\n", pthread_self(), numHostnameFilesServiced); // Print to stdout how many files each producer thread serviced
            fflush(stderr); // Flush the stderr log
            return NULL; // Return NULL (breaks out of infinite while loop)
        }
        pthread_mutex_unlock(&(data -> servicedFileCounterLock)); // Unlock the servicedFileCounter lock in case we didn't satisfy file counter if statement

        pthread_mutex_lock(&(data -> servicedFileCounterLock)); // Lock the servicedFileCounter lock again
        int fileToService = data -> servicedFileCounter; // Track the current file to service
        data -> servicedFileCounter = data -> servicedFileCounter + 1; // Increment the current file to service
        pthread_mutex_unlock(&(data -> servicedFileCounterLock)); // Release the servicedFileCounterlock again
        
        FILE* hostnameFile = fopen(data -> servicedFileArray[fileToService], "r"); // Define a file pointer to fopen where we open the current input file. Each requester has its own file stream object, so locking an fopen for these file objects is not necessary.
        int fdHostnameFile = access(data -> servicedFileArray[fileToService], F_OK | R_OK); // Call access to check file descriptor for openability and readability
        if (hostnameFile == NULL || fdHostnameFile != 0) // If we were unable to open the file (fp is null)
        {
            pthread_mutex_lock(&(data -> stdErrLock)); // Lock the stderr lock
            fprintf(stderr, "invalid file %s\n", data -> servicedFileArray[fileToService]); // Log to std err that we were unable to open a file
            fflush(stderr); // Flush the stderr log
            pthread_mutex_unlock(&(data -> stdErrLock)); // Release the stderr lock

            //numHostnameFilesServiced += 1; // Increase numHostnameFilesServiced by 1 since we still managed to service it (it just happened to point to null)
        }
        else // If the file was readable
        {
            char fileLine[MAX_NAME_LENGTH]; // Define a stack variable fileLine to track each line in the input file
            while (fgets(fileLine, MAX_NAME_LENGTH, hostnameFile) != NULL) // Call fgets to get each line from the file stream (make sure that it does not return NULL (no line present))
            {
                // Removes newlines from fileLine
                for (int i = 0; i < MAX_NAME_LENGTH; i++) // Loop through until MAX_NAME_LENGTH
                {
                    if (fileLine[i] == '\n') // If we encounter a newline character
                    {
                        fileLine[i] = '\0'; // Reset it to null terminator
                        break; // Break from for loop
                    }
                    if (fileLine[i] == '\0') // If we encounter a null terminator
                    {
                        break; // Break from for loop immediately
                    }
                }
                
                if (array_put(a, fileLine) != 0) // Call array_put on file line (place it into the shared array) and check it for error conditions
                {
                    // Standard prorgram end if array_put fails
                    pthread_mutex_lock(&(data -> stdErrLock)); // Lock the std err lock
                    fprintf(stderr, "Error: Could not put hostname into shared array.\n"); // Print to stderr that array_put unsuccessful
                    array_free(&(data -> shared_array)); // Free the shared array
                    free(data); // Free the struct
                    exit(-1); // Exit with status -1
                    pthread_mutex_unlock(&(data -> stdErrLock)); // Unlock the std err lock
                }

                pthread_mutex_lock(&(data -> servicedWriteLock)); // Lock ther servicedWriteLock 
                fprintf(data -> serviced, "%s\n", fileLine); // Write each hostname/fileLine out to the serviced.txt file using fprintf. Protect write access to log files from different threads
                pthread_mutex_unlock(&(data -> servicedWriteLock)); // Unlock the servicedWriteLock
            }
            fclose(hostnameFile); // Once we have read through the file, close the file pointer with fclose
            numHostnameFilesServiced = numHostnameFilesServiced + 1; // Increment number of hostname files serviced by 1
        }
    }

    return NULL; // Return NULL at the end
}


void* resolver(void* args)
{
    Buffer *data = (Buffer *) args; // Cast void pointer to buffer pointer (to be used when accessing passed in struct data)
    stack* a = &(data -> shared_array); // Declare pointer to shared array from struct (ease of acccess with array_get)

    char* gotHostname = malloc(MAX_NAME_LENGTH); // Malloc a string for each hostname we receive from array_get. Check malloc failure and exit if failed

    if (gotHostname == NULL)
    {
        fprintf(stderr, "Error: Malloc of gotHostname string failed, program terminating.\n");
        exit(-1); // Exit with status -1
    }

    char* IPGotHostname = malloc(MAX_IP_LENGTH); // Malloc an IP string for DNS lookup

    if (IPGotHostname == NULL)
    {
        fprintf(stderr, "Error: Malloc of IPGotHostname string failed, program terminating.\n");
        exit(-1); // Exit with status -1
    }

    int numHostnamesResolved = 0; // Declare counter per thread for number of hostnames resolved

    while (1) // Begin infinite while loop for consumers
    {
        // pthread_mutex_lock(&(data -> producersLeftLock));
        // pthread_mutex_lock(&(data -> shared_array.mutex));
        // if ((data -> producersLeft <= 0) && (data -> shared_array.counter <= 0))
        // {
        //     pthread_mutex_unlock(&(data -> shared_array.mutex));
        //     pthread_mutex_unlock(&(data -> producersLeftLock));

        //     printf("Thread %lu resolved %i hostnames.\n", pthread_self(), numHostnamesResolved);
        //     fflush(stderr);
        //     sem_post(&(a -> full));
        //     free(gotHostname);
        //     free(IPGotHostname);
        //     return NULL;    
        // }
        // pthread_mutex_unlock(&(data -> shared_array.mutex));
        // pthread_mutex_unlock(&(data -> producersLeftLock));

        // 1. while(1) enter the while loop. 
        // 2. FOr chekcing producers left and stack is empty (could be two separate helper functions)
        // 3. producersLeft function => return the number of producers you have left (checkRegisters)
        // 4. int num = deference *producersLeft (use producersLeftLock) return num
        // 5. isEmpty => lock (same one from shared array) tnt emp = b->stack->top == 0; unlock
        // 6. Resolver -> thread pool = while (1) if (!checkRequesters() == 1 && stackEmpty() == 1)

        if (array_get(a, &gotHostname) != 0) // Call array_get if the consumer was not able to exit 
        {
            pthread_mutex_lock(&(data -> stdErrLock)); // If unsuccessful, end the program. First lock stderrLock
            fprintf(stderr, "Error: Could not get hostname from shared array.\n"); // Print to std err that we could not get the hostname from shared array
            array_free(&(data -> shared_array)); // Call array_free to free the shared array
            free(data); // Free the struct
            exit(-1); // Exit with status -1
            pthread_mutex_unlock(&(data -> stdErrLock)); // Unlock the std err mutex
        }

        //printf("Got hostname: %s.\n", gotHostname);

        if (strcmp(gotHostname, "poison") == 0) // If we find a poison pill, consumer should exit
        {
            printf("thread %lx resolved %d hostnames\n", pthread_self(), numHostnamesResolved); // Print statement for consumer threads exit 
            fflush(stdout); // Stdout flush
            //sem_post(&(a -> full)); // Post to shared array full semaphore (ensures that each consumer thread can fall through when the previous consumer thread exits) 
            free(gotHostname); // Free the malloced gotHostname 
            free(IPGotHostname); // Free the malloced IPGotHostname
            return NULL; // Return NULL which will terminate the consumer thread
        }

        if (dnslookup(gotHostname, IPGotHostname, MAX_IP_LENGTH) == 0) // Call DNS lookup with the gotHostname, IP address string (Empty when DNSlookup called), MAX_IP_LENGTH. If successful...
        {
            numHostnamesResolved = numHostnamesResolved + 1; // Increment number of hostnames resolved by 1
            // printf("IP Address: %s.\n", IPGotHostname);

            pthread_mutex_lock(&(data -> resultsWriteLock)); // Lock the results file lock
            fprintf(data -> results, "%s", gotHostname); // Fprintf the hostname
            fprintf(data -> results, ", "); // Fprintf the comma
            fprintf(data -> results, "%s", IPGotHostname); // Fprintf IP string
            fprintf(data -> results, "\n"); // Fprintf a new line
            pthread_mutex_unlock(&(data -> resultsWriteLock));
        }
        else // If DNS lookup failed
        {
            pthread_mutex_lock(&(data -> resultsWriteLock)); // Lock the results file lock
            fprintf(data -> results, "%s", gotHostname); // Fprintf the hostname
            fprintf(data -> results, ", "); // Fprintf the comma
            fprintf(data -> results, "NOT_RESOLVED"); // Fprintf manually NOT_RESOLVED
            fprintf(data -> results, "\n"); // Fprintf a new line
            pthread_mutex_unlock(&(data -> resultsWriteLock)); // Unlock the results file lock
        }
    }

    return NULL; // Return NULL from the consumers thread
}

// Each thread handles one file
// Pass all of the file names
// Mutual exclusion for each new file
int main(int argc, char* argv[])
{
    struct timeval start; // Define start timeval struct
    struct timeval end; // Define end timeval struct

    gettimeofday(&start, NULL); // Start the timer for multi-threading

    if (argc < 6) // Validation of input, should not be less than or equal to 6 input args 
    {
        printf("Error: Need more arguments for multi-lookup.\n"); // Print that we need more args
        printf("Arguments should be in the form: multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ... ]\n"); // Print what form the arguments should be in
        return -1; // Exit with negative status code
    }

    // Use fprintf to print to stderr: https://stackoverflow.com/questions/39002052/how-i-can-print-to-stderr-in-c
    if (argc > MAX_INPUT_FILES + 5) // Validation of input, if there are more arguments than expected / too many input files
    {
        fprintf(stderr, "Error: Too many input files passed in.\n"); // Print too many input files passed in
        return -1; // Exit with negative status code
    }

    char* strtolPointer; // Storage string for strtol excess strings. Default value is null terminator

    int numRequesters = (int) strtol(argv[1], &strtolPointer, 10); // Convert argv[1] to integer (number of requester threads). Potentially unsafe, use strtol

    if (strtolPointer[0] != '\0') // strtolPointer should store whatever could not be converted to integer so check to see if it is not equal to the null terminator (something was stored in strtolPointer)
    {
		fprintf(stderr, "Error: Requester thread number uninterperable.\n"); // Requester threads uninterperable
		return -1; // Exit with negative status code
	}
    
    int numResolvers = (int) strtol(argv[2], &strtolPointer, 10); // Convert argv[2] to integer (number of resolver threads)
    
    if (strtolPointer[0] != '\0') // strtolPointer should store whatever could not be converted to integer so check to see if it is not equal to the null terminator (something was stored in strtolPointer)
    {
		fprintf(stderr, "Error: Resolver thread number uninterperable.\n"); // Resolver threads uninterperable
		return -1; // Exit with negative status code
	}

    if (numRequesters > MAX_REQUESTER_THREADS) // If the number of requester threads requested is greater than the maximum number we can have
    {
        fprintf(stderr, "Error: Too many requester threads requested. Multi-lookup can only handle %d requester threads\n", MAX_REQUESTER_THREADS); // Print to stderr that the user requested too many requester threads
        return -1; // Exit with a negative status code
    }

    if (numResolvers > MAX_RESOLVER_THREADS) // If the number of resolver threads requested is greater than the maximum number we can have
    {
        fprintf(stderr, "Error: Too many resolver threads requested. Multi-lookup can only handle %d resolver threads\n", MAX_RESOLVER_THREADS); // Print to stderr that the user requested too many resolver threads
        return -1; // Exit with negative status code
    }

    if (numRequesters <= 0) // If the number of requesters is below or equal to zero
    {
        fprintf(stderr, "Error: No requester threads requested.\n"); // Print to stderr that the user requested not enough requester threads
        return -1; // Exit with negative status code
    }

    if (numResolvers <= 0)
    {
        fprintf(stderr, "Error: No resolver threads requested.\n"); // Print to stderr that the user requested not enough resolver threads
        return -1; // Exit with negative status code
    }

    Buffer* multilookup_struct = (Buffer*) calloc(sizeof(Buffer), sizeof(Buffer)); // Define shared array data structure from part A. Use Calloc to initialize all struct member beforehand
    
    if (multilookup_struct == NULL) // Check if calloc failed
    {
        fprintf(stderr, "Error: Malloc of multilookup_struct failed, program terminating.\n"); // Print error message
        return -1; // Exit with status -1
    }
    
    stack* a = &(multilookup_struct -> shared_array);

    multilookup_struct -> serviced = fopen(argv[3], "w"); // Call fopen to open the file specified with argv[3] if it exists and is writeable or create it if it does not exist (serviced file for requesters)
    int fdServiced = access(argv[3], F_OK | W_OK); // Call access to check file descriptor for openability and writability
    
    multilookup_struct -> results = fopen(argv[4], "w"); // Call fopen to open the file specified with argv[4] if it exists and is writeable or create it if it does not exist (results file for resolvers)
    int fdResults = access(argv[4], F_OK | W_OK); // Call access to check file descriptor for openability and writability

    if (multilookup_struct -> serviced == NULL || fdServiced != 0 ||  multilookup_struct -> results == NULL || fdResults != 0) // Check if either serviced file pointer or results file pointer is still null
    {
        fprintf(stderr, "Error: Could not open or create log files.\n"); // Log that we could not open or create requested files to stderr
        return -1; // Exit with negative status code
    }

    if (array_init(&(multilookup_struct -> shared_array)) != 0) // Initialize shared stack array
    {
        printf("Buffer initialization not successful.\n"); // If initialization was not successful, print that it was not
        return -1; // Exit with negative status code
    }

    multilookup_struct -> servicedFileCounter = 0; // Initialize the serviced file counter to 0
    multilookup_struct -> numFilesToService = argc - 5; // Initialize the number of files to service to argc - 5
    multilookup_struct -> producersLeft = numRequesters; // Initialize producers left to numRequesters (command line arg)

    for (int i = 5; i < argc; i++) // Loop through starting from the first input file to the very last index of command line arguments (argc)
    {
        multilookup_struct -> servicedFileArray[i - 5] = argv[i]; // Store each hostname into the serviced file array using argv
    }

    // Create requester and resolver thread pools with command line args
    pthread_t requesters[numRequesters]; 
    pthread_t resolvers[numResolvers];

    // Initialize the shared mutex locks
    pthread_mutex_init(&(multilookup_struct -> stdErrLock), NULL); //This will be used to moderate thread access to stderr
    pthread_mutex_init(&(multilookup_struct -> stdOutLock), NULL); //This will be used to moderate thread access to stdout
    pthread_mutex_init(&(multilookup_struct -> producersLeftLock), NULL); // Producers left lock
    pthread_mutex_init(&(multilookup_struct -> servicedFileCounterLock), NULL); // Serviced file counter array lock
    pthread_mutex_init(&(multilookup_struct -> servicedWriteLock), NULL); // Serviced.txt file write lock
    pthread_mutex_init(&(multilookup_struct -> resultsWriteLock), NULL); // Results.txt file write lock

    for (int i = 0; i < numRequesters; i++) // Loop through number of requesters command line arg
    {
        if (pthread_create(&requesters[i], NULL, &requester, multilookup_struct) != 0) // Call pthread_create to create each requester thread (address of requester thread, tid, NULL for default thread attributes, requester is the start routine, multilookup-struct passed to start routine as argument)
        {
            printf("Error: Failed to create requester threads.\n"); // If pthread_create fails, print an error message 
            return -1;
        }
    }

    for (int i = 0; i < numResolvers; i++) // Loop through number of resolvers command line arg
    {
        if (pthread_create(&resolvers[i], NULL, &resolver, multilookup_struct) != 0) // Call pthread_create to create each resolver thread (address of resolver thread, tid, NULL for default thread attributes, resolver is the start routine, multilookup-struct passed to start routine as argument)
        {
            printf("Error: Failed to create resolver threads.\n"); // If pthread_create fails, print an error message
            return -1;
        }
    }

    // Find out the difference between pthread_join = "block on the main routine, until they finish" and pthread_detach = "main routine keeps running and not waiting on threads to finish. Terminate on main routine exit"
    for (int i = 0; i < numRequesters; i++) // Loop through requester pool number
    {
        if (pthread_join(requesters[i], NULL) != 0) // Call pthread_join on each requester thread which blocks the calling thread until the specified tid thread terminates (Ensures all requester threads terminate). Arguments: (thread id, NULL)
        {
            printf("Error: Failed to join requesters.\n"); // If pthread_join fails, log an error message via printf
            return -1;
        }
    }

    // printf("PRODUCERS ALL JOINED.\n");

    while (a -> counter != 0); // Wait until stack is empty. Resolvers keep working to clear it out
    for (int i = 0; i < numResolvers; i++) // Loop through number of resolvers
    {
        if (array_put(a, "poison") != 0) // Put in poison pills into shared array to clear resolvers
        {
            pthread_mutex_lock(&(multilookup_struct -> stdErrLock)); // Lock the std err lock
            fprintf(stderr, "Error: Could not put hostname into shared array.\n"); // Print error message
            array_free(&(multilookup_struct -> shared_array)); // Free the shared array
            free(multilookup_struct); // Free the struct
            exit(-1); // Exit with negative return status
            pthread_mutex_unlock(&(multilookup_struct -> stdErrLock)); // Unlock the std err lock
        }
    }

    for (int i = 0; i < numResolvers; i++) // Loop through the number of resolvers to join them
    {
        if (pthread_join(resolvers[i], NULL) != 0) // Call pthread_join on each resolver tid and NULL as second argument
        {
            printf("Error: Failed to join resolvers.\n"); // If pthread_join fails, we print an error message saying resolvers failed
            return -1;
        }
    }

    //printf("CONSUMERS ALL JOINED.\n");

    fclose(multilookup_struct -> serviced); // Close the serviced file pointer
    fclose(multilookup_struct -> results); // Close the results file pointer

    // Destroy mutex locks after use
    pthread_mutex_destroy(&(multilookup_struct -> stdErrLock)); // Destroy std err lock
    pthread_mutex_destroy(&(multilookup_struct -> stdOutLock)); // Destroy std out lock
    pthread_mutex_destroy(&(multilookup_struct -> producersLeftLock)); // Destroy producers left lock
    pthread_mutex_destroy(&(multilookup_struct -> servicedFileCounterLock)); // Destroy serviced file counter lock
    pthread_mutex_destroy(&(multilookup_struct -> servicedWriteLock)); // Destroy serviced write lock
    pthread_mutex_destroy(&(multilookup_struct -> resultsWriteLock)); // Destroy results write lock

    array_free(&(multilookup_struct -> shared_array)); // Call array_free on the shared array part of multilookup_struct
    free(multilookup_struct); // Free the multilookup_struct 

    gettimeofday(&end, NULL); // End the timer 
    float timeElapsed = (end.tv_sec - start.tv_sec) + 1e-6*(end.tv_usec - start.tv_usec); // Calculate elapsed time
    printf("./multi-lookup: total time is %f seconds\n", timeElapsed); // Print elapsed time for ./multi-lookup

    return 0; // Return 0 on success from main
}
