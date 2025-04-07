//page  => memory page
//slab  => manager of a memory page
//chunk => minimum unit for allocation


#include "slab.h"
#include "function.h"
#include <string.h>
#include <time.h>


static char g_memory[PAGE_SIZE] __attribute__((aligned(sizeof(int))));

// like a manager of all container, allocate/recycle struct containers.
static Slab g_containerSlab;

//slab mutex
static pthread_mutex_t slab_mutex = PTHREAD_MUTEX_INITIALIZER;

//slabLog
extern FILE* g_memSlabLog;


static void get_timestamp(char* buffer, size_t len) {
    time_t now = time(NULL);
    struct tm* tinfo = localtime(&now);
    strftime(buffer, len, "%Y-%m-%d %H:%M:%S", tinfo);
}

// record the current status to mem_slab.txt
static void logSlabState(const char* operation, Slab* slab) {
    if (!g_memSlabLog) return; // if failed to open slabLog file, then return

    // get the length of freeList
    int free_count = 0;
    SlabEntry* p = slab->freeList;
    while (p) {
        free_count++;
        p = p->next;
    }
    int allocated = slab->totalChunks - free_count;

    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    fprintf(g_memSlabLog,
            "[%s] %s => total=%d, allocated=%d, free=%d\n",
            timestamp,
            operation,
            slab->totalChunks,
            allocated,
            free_count
    );
    fflush(g_memSlabLog);
}




void slabInit(Slab* slab, void* pageMemory, int chunkSize)
{
    slab->pageStart   = pageMemory;
    slab->chunkSize   = chunkSize;
    slab->totalChunks = PAGE_SIZE / chunkSize;

    // Set freeList to the start of the page
    slab->freeList    = (SlabEntry*) pageMemory;

    // Split this page into "totalChunks" chunks
    // Then link them using SlabEntry
    // like: [0] -> [chunkSize] -> [chunkSize*2] ->
    for (int i = 0; i < slab->totalChunks - 1; i++) {
        SlabEntry* curr = (SlabEntry*)((char*)pageMemory + i * chunkSize);
        curr->next       = (SlabEntry*)((char*)pageMemory + (i + 1) * chunkSize);
    }

    // Set the next pointer of the last chunk to NULL
    SlabEntry* last = (SlabEntry*)((char*)pageMemory + (slab->totalChunks - 1) * chunkSize);
    last->next = NULL;
}


void* slabAlloc(Slab* slab)
{
    pthread_mutex_lock(&slab_mutex);  // lock the available list

    // if no available chunk
    if (slab->freeList == NULL) {
        pthread_mutex_unlock(&slab_mutex);
        return NULL;
    }

    // allocate first available chunk
    SlabEntry* chunk = slab->freeList;
    slab->freeList   = chunk->next;

    pthread_mutex_unlock(&slab_mutex);

    logSlabState("ALLOC", slab);//record alloc operation to log
    return chunk; // return the address of this chunk
}


void slabFree(Slab* slab, void* ptr)
{
    if (!ptr) {
        return;
    }

    pthread_mutex_lock(&slab_mutex);
    SlabEntry* chunk = (SlabEntry*) ptr;
    chunk->next      = slab->freeList;
    slab->freeList   = chunk;
    pthread_mutex_unlock(&slab_mutex);

    logSlabState("FREE", slab);
}


void initContainerSlab()
{
    // chunkSize = sizeof(struct containers)
    slabInit(&g_containerSlab, g_memory, sizeof(struct containers));
}


struct containers* containerAlloc()
{
    return (struct containers*) slabAlloc(&g_containerSlab);
}


void containerFree(struct containers* c)
{
    slabFree(&g_containerSlab, (void*)c);
}
