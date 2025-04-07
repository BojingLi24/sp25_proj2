#ifndef SLABTEST_FUNCTION_H
#define SLABTEST_FUNCTION_H

#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define PORT_BUFFER_SIZE 20

extern int g_numProducers;
extern FILE* g_portLog;
extern FILE* g_distributionLog;
extern FILE* g_memSlabLog;

typedef struct containers {
    int containerID;            // 4 bytes
    int companyID;              // 4 bytes
    int arrivalMode;            // 4 bytes
    int departureMode;          // 4 bytes
    time_t prod_timestamp;      // 8 bytes on 64-bit
    time_t cons_timestamp;      // 8 bytes on 64-bit
} container_t;

extern container_t* port_buffer[PORT_BUFFER_SIZE];

void* producer_function(void* arg);
void* consumer_function(void* arg);

void simulate_work_with_bubble_sort();

#endif
