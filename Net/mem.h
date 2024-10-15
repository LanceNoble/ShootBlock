#pragma once

#define DEBUG_MALLOC(sz) mem_track_alloc(sz, __FILE__, __LINE__)
#define DEBUG_FREE(ptr, n) mem_track_free(ptr, sizeof(*ptr) * n, __FILE__, __LINE__)

extern void mem_track_create();
extern void* mem_track_alloc(unsigned int sz, char* file, unsigned int line);
extern void mem_track_free(void* ptr, unsigned int sz, char* file, unsigned int line);
extern void mem_track_show();
extern void mem_track_destroy();