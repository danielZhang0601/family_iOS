#include "sys/sys_mem.h"
#include <stdlib.h>


void* sys_my_malloc(int size, const char *file, const char *func, int line)
{
	return malloc(size); 
}

void sys_my_free(void *ptr)
{
	free(ptr);
}

#ifdef NDEBUG

void* sys_malloc(int size)
{
	return malloc(size);
}

void sys_free(void *ptr)
{
	free(ptr);
}

#endif
