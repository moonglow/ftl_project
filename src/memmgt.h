#pragma once
#include <stdlib.h>

void *ftl_malloc(size_t size);
void ftl_free(void *ptr);
void *ftl_memset(void *dst, int c, size_t size);
void *ftl_memcpy(void *dst, const void *src, size_t num);