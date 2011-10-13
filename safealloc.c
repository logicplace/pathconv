//MIT Licensed

#include <stdlib.h>
#include "safealloc.h"

#define INITSAFEALLOC if(safe_allocs == NULL)safe_allocs = (alloc*)malloc(0)

typedef struct _alloc {
	void *ptr;
	size_t size;
} alloc;

alloc *safe_allocs = NULL; int safe_len = 0;
void* safe_malloc(size_t size){
	INITSAFEALLOC;
	safe_allocs = realloc(safe_allocs, (++safe_len) * sizeof(alloc));
	safe_allocs[safe_len-1].size = size;
	return (safe_allocs[safe_len-1].ptr = malloc(size));
}

void* safe_realloc(void* ptr, size_t size){
	int i;
	INITSAFEALLOC;
	for(i=0;i<safe_len;++i){
		if(safe_allocs[i].ptr == ptr){
			safe_allocs[i].size = size;
			return (safe_allocs[i].ptr = realloc(ptr,size));
		}
	}
	//This wasn't handled by us before
	safe_allocs = realloc(safe_allocs, (++safe_len) * sizeof(alloc));
	safe_allocs[safe_len-1].size = size;
	return (safe_allocs[safe_len-1].ptr = realloc(ptr,size));
}

void safe_free(void* ptr){
	int i;
	for(i=0;i<safe_len;++i){
		if(safe_allocs[i].ptr == ptr){
			free(ptr);
			safe_allocs[i].ptr = safe_allocs[--safe_len].ptr;
			safe_allocs[i].size = safe_allocs[safe_len].size;
			safe_allocs = realloc(safe_allocs, safe_len * sizeof(alloc));
			return;
		}
	}
	return;
}

//Call this at the end of main
void safe_freeall(){
	int i;
	for(i=0;i<safe_len;++i){
		free(safe_allocs[i].ptr);
	}
	free(safe_allocs);
	return;
}

size_t safe_length(void* ptr, int div){
	int i;
	for(i=0;i<safe_len;++i){
		if(safe_allocs[i].ptr == ptr){
			return safe_allocs[i].size / div;
		}
	}
	return 0;
}

size_t safe_size(void* ptr){
	return safe_length(ptr,1);
}
