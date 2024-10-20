// Track allocated/freed memory

#pragma once

// Seamlessly switch between debug and no-debug memory
#ifdef MEM_DEBUG_MODE
extern void mem_track_create();
extern void* mem_track_alloc(const unsigned int sz, const char* file, const unsigned int line);
extern void mem_track_free(void** ptr, const char* file, const unsigned int line);
extern void mem_track_show();
extern void mem_track_destroy();
#define malloc(sz) mem_track_alloc(sz, __FILE__, __LINE__)
#define free(ptr) mem_track_free(&(ptr), __FILE__, __LINE__)
#endif // MEM_DEBUG_MODE