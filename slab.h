#ifndef SLAB_H
#define SLAB_H

#include <stdio.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

// Size of each memory page
#define PAGE_SIZE 8192

typedef struct SlabEntry {
    struct SlabEntry* next;
} SlabEntry;


typedef struct Slab {
    void*      pageStart;       //start of a memory page
    SlabEntry* freeList;        //list of all available chunks
    int        chunkSize;
    int        totalChunks;
} Slab;


void slabInit(Slab* slab, void* pageMemory, int chunkSize);
void* slabAlloc(Slab* slab);
void slabFree(Slab* slab, void* ptr);
void initContainerSlab();
struct containers* containerAlloc();
void containerFree(struct containers* c);

#ifdef __cplusplus
}

#endif
#endif // SLAB_H
