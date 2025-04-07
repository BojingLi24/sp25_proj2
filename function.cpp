#include "function.h"
#include "slab.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#define NUM_COMPANIES 5
#define BATCH_SIZE    10  //when consumer have BATCH_SIZE containers for a company, then ship this batch and free slab

//assign one lock for each company, to prevent multiple consumers visit one company accumulated data at the same time
static pthread_mutex_t g_company_mutex[NUM_COMPANIES] = { 
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER,
    PTHREAD_MUTEX_INITIALIZER
};


static container_t* g_companyAccum[NUM_COMPANIES][BATCH_SIZE]; // count of containers for each company that are ready to leave the port on trucks
static int g_companyCount[NUM_COMPANIES] = {0};        // count of containers for each company that have arrived in the port

extern sem_t empty_slots;
extern sem_t full_slots;
extern pthread_mutex_t buffer_mutex;

extern container_t* port_buffer[PORT_BUFFER_SIZE];//this is the port buffer, the main producer consumer circular array
extern int producer_in;//this is the index position where a producer thread is going to write next
extern int consumer_out;// this is the index position where a consumer thread is going to read next

extern FILE* g_fp;
extern pthread_mutex_t g_file_mutex;//a producer thread must acquire this mutex before accessing the input file

extern int g_producerDoneCount;//count of how many producer thread have finished the while loop and ready to exit thread
extern pthread_mutex_t g_producerDoneLock;//a producer thread must acquire this mutex before accessing g_producerDoneCount

extern FILE* g_portLog;
extern FILE* g_distributionLog;
extern FILE* g_memSlabLog;

void simulate_work_with_bubble_sort();

static void get_timestamp(char* buffer, size_t len) {
    time_t now = time(NULL);
    struct tm* tinfo = localtime(&now);
    strftime(buffer, len, "%Y-%m-%d %H:%M:%S", tinfo);
}

//when containers leave the port on trucks, those instances are freed from the memory slab
void free_batch_for_company(int cIndex) {
    pthread_mutex_lock(&g_company_mutex[cIndex]);
    for (int i = 0; i < g_companyCount[cIndex]; i++) {
        if (g_companyAccum[cIndex][i]) {
            containerFree(g_companyAccum[cIndex][i]);
            g_companyAccum[cIndex][i] = NULL;
        }
    }
    g_companyCount[cIndex] = 0;//this array keeps count of how many containers are in the distribution arrays, ready to leave on trucks
    pthread_mutex_unlock(&g_company_mutex[cIndex]);
}



void* producer_function(void* arg) {
    int producer_id = *(int*)arg;
    printf("Producer %d: Thread started.\n", producer_id);

    char line[256];
    while (1) {
        // lock the input file while reading
        pthread_mutex_lock(&g_file_mutex);
        if (!fgets(line, sizeof(line), g_fp)) {
            pthread_mutex_unlock(&g_file_mutex);
            break; // if finish reading input file, break the while loop and kill this producer thread
        }
        
        //unlock the input file
        pthread_mutex_unlock(&g_file_mutex);

        // use sscanf to parse one line, if not the correct format, continue(ignore this line)
        int contID, cmpID, arrMode, depMode;
        if (sscanf(line, "%d %d %d %d", &contID, &cmpID, &arrMode, &depMode) != 4) {
            fprintf(stderr, "Producer %d: parse error: %s\n", producer_id, line);
            continue;
        }

        // use bubble sort to simulate work (for example CPU)
        simulate_work_with_bubble_sort();

        // Allocate a new container struct from slab
        container_t* c = containerAlloc();
        if (!c) {
            fprintf(stderr, "Producer %d: slabAlloc failed.\n", producer_id);
            // if failed, break the while loop and kill producer thread
            break;
        }
        
        //assign values read from input file to new container struct
        c->containerID  = contID;
        c->companyID    = cmpID;
        c->arrivalMode  = arrMode;
        c->departureMode= depMode;
        c->prod_timestamp = time(NULL); //record the timestamp of producer

        //TODO
        // producer may wait until there is an available slot
        // if empty_slots>0 => decrement empty_slots and proceed
        // if empty_slots=0 => WAIT until empty_slots>0
 

        //TODO
        // acquire the mutex, then copy the pointer c to the circular buffer
        // producer_in is the index of producer



        // log the event in the portLog with timestamp
        if (g_portLog) {
            char timebuf[64];
            strftime(timebuf, sizeof(timebuf),
                     "%Y-%m-%d %H:%M:%S", localtime(&c->prod_timestamp));
            fprintf(g_portLog,
                    "[%s] Producer %d: Put container => "
                    "(containerID=%d, companyID=%d, arrivalMode=%d, departureMode=%d)\n",
                    timebuf, producer_id,
                    c->containerID, c->companyID, c->arrivalMode, c->departureMode
            );
            fflush(g_portLog);
        }

        //TODO
        // signal consumer threads there is new data
        // increment full_slots +=1

        
    }//end while loop

    // update the count of producer threads that have finished the while loop 
    pthread_mutex_lock(&g_producerDoneLock);
    g_producerDoneCount++;
    pthread_mutex_unlock(&g_producerDoneLock);

    printf("Producer %d: Finished reading.\n", producer_id);
    pthread_exit(NULL);
}


void* consumer_function(void* arg) {
    int consumer_id = *(int*)arg;
    printf("Consumer %d: Thread started.\n", consumer_id);

    while (1) {
        // Check if all producers have finished + if there is any data left
        pthread_mutex_lock(&g_producerDoneLock);
        int doneCount = g_producerDoneCount;
        pthread_mutex_unlock(&g_producerDoneLock);

        // Check if there is data to be consumed in the port buffer
        // get the current value of full_slots
        int fullVal;
        sem_getvalue(&full_slots, &fullVal);

        // If all producers are finished and the buffer is empty, then there is no data to read, exit thread
        if (doneCount == g_numProducers && fullVal == 0) {
            printf("Consumer %d: All producers done & no items in port buffer, exiting.\n",
                   consumer_id);
            break;
        }
        
        //TODO
        //consumer thread may need to wait for a full slot
 

        //TODO
        //consumer threads must acquire the mutex for the port buffer
        //copy the pointer to the company's distribution array
        //remove the pointer from the slot in the port buffer
        //release the mutex

        
        simulate_work_with_bubble_sort();
        c->cons_timestamp = time(NULL);

        //TODO
        //release an empty slot
        //signal producer thread that may be waiting for an empty slot



        // record it to portLog
        if (g_portLog) {
            char timebuf[64];
            strftime(timebuf, sizeof(timebuf),
                     "%Y-%m-%d %H:%M:%S", localtime(&c->cons_timestamp));
            fprintf(g_portLog,
                    "[%s] Consumer %d: Remove container => "
                    "(containerID=%d, companyID=%d, arrivalMode=%d, departureMode=%d)\n",
                    timebuf, consumer_id,
                    c->containerID, c->companyID, c->arrivalMode, c->departureMode
            );
            fflush(g_portLog);
        }

        // record it to distributionLog
        if (g_distributionLog) {
            char timebuf[64];
            strftime(timebuf, sizeof(timebuf),
                     "%Y-%m-%d %H:%M:%S", localtime(&c->cons_timestamp));
            fprintf(g_distributionLog,
                    "[%s] Consumer %d: Distributed container => "
                    "(containerID=%d, companyID=%d, arrivalMode=%d, departureMode=%d)\n",
                    timebuf, consumer_id,
                    c->containerID, c->companyID, c->arrivalMode, c->departureMode
            );
            fflush(g_distributionLog);
        }
        
        
        int cid = c->companyID;
        int cIndex = cid - 1;// company1=>index0
        
        pthread_mutex_lock(&g_company_mutex[cIndex]);
        int idx = g_companyCount[cIndex];
        g_companyAccum[cIndex][idx] = c;
        g_companyCount[cIndex]++;
        pthread_mutex_unlock(&g_company_mutex[cIndex]);
        
	if (g_companyCount[cIndex] == BATCH_SIZE) {
            free_batch_for_company(cIndex);
        }
        
    usleep(200000);
        
    }//end consumer thread while loop
    
    //all remaining trucks leave the port and all container instances are freed from the memory slab
    for (int cIndex = 0; cIndex < NUM_COMPANIES; cIndex++) {
        if (g_companyCount[cIndex] > 0) {
            	free_batch_for_company(cIndex);
        }
    }
        
    
    printf("Consumer %d: Thread exiting, leftover containers freed.\n", consumer_id);
    pthread_exit(NULL);
}


void simulate_work_with_bubble_sort() {
    int size = rand() % 10001 + 8000; // [1000, 2000]

    int* arr = (int*)malloc(size * sizeof(int));
    if (!arr) {
        printf("error in malloc pointer arr");
        return;
    }
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 10000;  // 0~9999
    }

    // bubble sort
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
    free(arr);
}
