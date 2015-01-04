#include "memmgt.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static uint32_t alloc_count = 0;
static uint32_t free_count = 0;

void *ftl_malloc( size_t size )
{
	++alloc_count;
	return malloc( size );
}

void ftl_free(void *ptr)
{
	if (ptr)
	{
		++free_count;
		free(ptr);
	}
}

void *ftl_memset(void *dst, int c, size_t size)
{
	return memset(dst, c, size);
}

void *ftl_memcpy(void *dst, const void *src, size_t num)
{
	return memcpy(dst, src, num);
}

void check_for_mem_leaks(void)
{
	printf("malloc = %d\nfree = %d\n", alloc_count, free_count);
	if((alloc_count- free_count) != 0)
	{
		printf("warning!!! check for leaks!\n");
	}
}
