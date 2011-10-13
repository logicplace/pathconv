//MIT Licensed

#ifndef __SAFEALLOC_H__
#define __SAFEALLOC_H__

void* safe_malloc(size_t size);
void* safe_realloc(void* ptr, size_t size);
void safe_free(void* ptr);
void safe_freeall();
size_t safe_length(void* ptr, int div);
size_t safe_size(void* ptr);

#endif //__SAFEALLOC_H__
