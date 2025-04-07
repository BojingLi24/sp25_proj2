/*
to compile: 

gcc *cpp -lpthread -lrt -o proj2

*/
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "function.h"
#include "slab.h"

#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 6

FILE* g_portLog = NULL;
FILE* g_distributionLog = NULL;
FILE* g_memSlabLog = NULL;

sem_t empty_slots;
sem_t full_slots;
pthread_mutex_t buffer_mutex;


container_t* port_buffer[PORT_BUFFER_SIZE] = {NULL};
int producer_in = 0;
int consumer_out = 0;

// file mutex lock()
// prevent two producers from opening input file and reading the same line at the same time
FILE* g_fp = NULL;
pthread_mutex_t g_file_mutex = PTHREAD_MUTEX_INITIALIZER;

// g_producerDoneCount: producers that have finished scanning the input file and hence their work, and are ready to exit the thread
// consumer threads will check this variable before exiting
int g_producerDoneCount = 0;
int g_numProducers = NUM_PRODUCERS;
//mutex to protect the global variable above
pthread_mutex_t g_producerDoneLock = PTHREAD_MUTEX_INITIALIZER;

int main() {
    // open input file stream
    g_fp = fopen("input.txt", "r");
    if (!g_fp) {
        fprintf(stderr, "Error: cannot open input.txt\n");
        return 1;
    }
    // ignore first line of input file
    int ch;
    while ((ch = fgetc(g_fp)) != EOF) {
        if (ch == '\n') break;
    }

    //open log file streams
    g_portLog        = fopen("port_log.txt", "w");
    g_distributionLog= fopen("distribution_log.txt", "w");
    g_memSlabLog     = fopen("mem_slab.txt", "w");

    if (!g_portLog || !g_distributionLog || !g_memSlabLog) {
        fprintf(stderr, "Error: cannot open one of the log files\n");
        return 1;
    }


    // initialize slab
    initContainerSlab();

    // initialize mutex and semaphores for the port buffer
    // 2nd parameter 0 means thread-level; 3rd parameter means init value
    sem_init(&empty_slots, 0, PORT_BUFFER_SIZE);
    sem_init(&full_slots, 0, 0);
    pthread_mutex_init(&buffer_mutex, NULL);

    // create arrays for producer and consumer threads
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];

    // create producer threads
    int producer_ids[NUM_PRODUCERS];
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producer_ids[i] = i;
        pthread_create(&producers[i], NULL, producer_function, &producer_ids[i]);
    }

    // create consumer threads
    int consumer_ids[NUM_CONSUMERS];
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumer_ids[i] = i;
        pthread_create(&consumers[i], NULL, consumer_function, &consumer_ids[i]);
    }

    // wait until all threads are finished
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    // destroy resources, free memory
    fclose(g_fp);
    fclose(g_portLog);
    fclose(g_distributionLog);
    fclose(g_memSlabLog);

    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
    pthread_mutex_destroy(&buffer_mutex);
    pthread_mutex_destroy(&g_file_mutex);
    pthread_mutex_destroy(&g_producerDoneLock);

    return 0;
}
