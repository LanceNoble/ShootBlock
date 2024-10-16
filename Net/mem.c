#include "mem.h"

// Ensure that the actual <stdlib.h> calls to malloc and free are used in this file
#ifdef MEM_DEBUG_MODE
#undef malloc
#undef free
#endif // MEM_DEBUG_MODE

#include <stdlib.h>
#include <stdio.h>

#define ALLOC 0
#define FREE 1

struct MemEvt {
	const char* file;
	unsigned int line;
	unsigned int act;
	unsigned int sz;
	void* ptr;
};

static unsigned int numBytes = 0;

static unsigned int numAllocEvts = 0;
static unsigned int szAllocEvts = 1;
static struct MemEvt* allocEvts = NULL;

static unsigned int numFreeEvts = 0;
static unsigned int szFreeEvts = 1;
static struct MemEvt* freeEvts = NULL;

static int mem_track_resize(struct MemEvt** evts, unsigned int* numEvts, unsigned int* szEvts) {
	if ((*numEvts) + 1 > *szEvts) {
		(*szEvts) *= 2;
		struct MemEvt* reallocEvts = (struct MemEvt*)(realloc((*evts), (*szEvts) * sizeof(struct MemEvt)));
		if (reallocEvts == NULL)
			return 1;
		*evts = reallocEvts;
	}
	return 0;
}

extern void mem_track_create() {
	if (allocEvts != NULL)
		return;
	allocEvts = malloc(szAllocEvts * sizeof(struct MemEvt));
	freeEvts = malloc(szFreeEvts * sizeof(struct MemEvt));
	if (allocEvts == NULL || freeEvts == NULL)
		return;
}

extern void* mem_track_alloc(const unsigned int sz, const char* file, const unsigned int line) {
	if (allocEvts == NULL)
		return NULL;

	if (mem_track_resize(&allocEvts, &numAllocEvts, &szAllocEvts) == 1)
		return NULL;

	void* ptr = malloc(sz);
	if (ptr == NULL)
		return NULL;

	allocEvts[numAllocEvts].file = file;
	allocEvts[numAllocEvts].line = line;
	allocEvts[numAllocEvts].act = ALLOC;
	allocEvts[numAllocEvts].sz = sz;
	allocEvts[numAllocEvts].ptr = ptr;

	numBytes += sz;
	numAllocEvts++;

	return ptr;
}

extern void mem_track_free(void** ptr, const char* file, const unsigned int line) {
	if (allocEvts == NULL)
		return;

	if (mem_track_resize(&freeEvts, &numFreeEvts, &szFreeEvts) == 1)
		return;

	if (*ptr == NULL)
		return;

	unsigned int i = 0;
	while (i < numAllocEvts) {
		if (allocEvts[i].ptr == *ptr)
			break;
		i++;
	}
	if (i == numAllocEvts)
		return;

	freeEvts[numFreeEvts].file = file;
	freeEvts[numFreeEvts].line = line;
	freeEvts[numFreeEvts].act = FREE;
	freeEvts[numFreeEvts].sz = allocEvts[i].sz;
	freeEvts[numFreeEvts].ptr = *ptr;

	numBytes -= allocEvts[i].sz;
	numFreeEvts++;

	free(*ptr);
	*ptr = NULL;
}

extern void mem_track_show() {
	printf("You allocated %lu times\n", numAllocEvts);
	printf("You freed %lu times\n", numFreeEvts);
	printf("%lu bytes remain\n", numBytes);
	/*
	unsigned int iEvent = 0;

	while (iEvent < numAllocEvts) {
		printf("File: %s\n", allocEvts[iEvent].file);
		printf("Line: %lu\n", allocEvts[iEvent].line);
		if (allocEvts[iEvent].act == ALLOC)
			printf("ALLOCATED ");
		else if (allocEvts[iEvent].act == FREE)
			printf("FREED ");
		printf("%lu Bytes at Address %p (%lu Bytes Remain)\n", allocEvts[iEvent].sz, allocEvts[iEvent].ptr, numBytes);
		printf("\n");
		
		iEvent++;
	}
	*/
}

extern void mem_track_destroy() {
	numBytes = 0;

	numAllocEvts = 0;
	szAllocEvts = 1;

	numFreeEvts = 0;
	szFreeEvts = 1;

	free(allocEvts);
	allocEvts = NULL;
	
	free(freeEvts);
	freeEvts = NULL;
}