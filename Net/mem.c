#include "mem.h"

#include <stdlib.h>
#include <stdio.h>

enum MemEvtAct {
	ALLOC = 0,
	FREE = 1
};

struct MemEvt {
	char* file;
	unsigned int line;
	enum MemEvtAct act;
	unsigned int sz;
	void* ptr;
};

static unsigned int numMemEvts = 0;

static unsigned int szMemEvts = 1;
static struct MemEvt* memEvts = NULL;

static int mem_track_resize() {
	numMemEvts++;
	if (numMemEvts > szMemEvts) {
		szMemEvts *= 2;
		auto struct MemEvt* reallocMemEvts = (struct MemEvt*)(realloc(memEvts, szMemEvts * sizeof(struct MemEvt)));
		if (reallocMemEvts == NULL) {
			numMemEvts--;
			return 1;
		}
		memEvts = reallocMemEvts;
	}
	return 0;
}

extern void mem_track_create() {
	if (memEvts != NULL)
		return;
	memEvts = malloc(szMemEvts * sizeof(struct MemEvt));
	if (memEvts == NULL)
		return;
}

extern void* mem_track_alloc(unsigned int sz, char* file, unsigned int line) {
	if (memEvts == NULL)
		return NULL;

	if (mem_track_resize() == 1)
		return NULL;

	auto void* ptr = malloc(sz);
	memEvts[numMemEvts - 1].file = file;
	memEvts[numMemEvts - 1].line = line;
	memEvts[numMemEvts - 1].act = ALLOC;
	if (ptr == NULL)
		memEvts[numMemEvts - 1].sz = 0;
	else
		memEvts[numMemEvts - 1].sz = sz;
	memEvts[numMemEvts - 1].ptr = ptr;

	return ptr;
}

extern void mem_track_free(void* ptr, unsigned int sz, char* file, unsigned int line) {
	if (memEvts == NULL)
		return;

	if (mem_track_resize() == 1)
		return;

	memEvts[numMemEvts - 1].file = file;
	memEvts[numMemEvts - 1].line = line;
	memEvts[numMemEvts - 1].act = FREE;
	memEvts[numMemEvts - 1].sz = sz;
	memEvts[numMemEvts - 1].ptr = ptr;
	free(ptr);
}

void mem_track_show() {
	unsigned int iEvent = 0;

	while (iEvent < numMemEvts) {
		printf("File: %s\n", memEvts[iEvent].file);
		printf("Line: %lu\n", memEvts[iEvent].line);
		printf("Action: ");
		if (memEvts[iEvent].act == ALLOC)
			printf("ALLOC\n");
		else if (memEvts[iEvent].act == FREE)
			printf("FREE\n");
		printf("Size: %lu\n", memEvts[iEvent].sz);
		printf("Address: %p\n", memEvts[iEvent].ptr);
		printf("\n");
		
		iEvent++;
	}
}

extern void mem_track_destroy() {
	numMemEvts = 0;
	szMemEvts = 1;

	free(memEvts);
	memEvts = NULL;
}