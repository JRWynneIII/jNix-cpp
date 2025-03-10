#ifndef _STRING_H
#define _STRING_H 1
 
#include <sys/cdefs.h>
#include <stdbool.h>
#include <stddef.h>
 
#ifdef __cplusplus
extern "C" {
#endif
 
int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
size_t strlen(const char*);
bool strcmp(char*, char*);
 
#ifdef __cplusplus
}
#endif
 
#endif
