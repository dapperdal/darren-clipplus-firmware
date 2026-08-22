#ifndef CLIPPLUS_MEMORY_H
#define CLIPPLUS_MEMORY_H

#include <stddef.h>

void *clipplus_memset(void *destination, int value, size_t count);
size_t clipplus_strlen(const char *text);
const char *clipplus_strrchr(const char *text, int value);
char *clipplus_strncpy(char *destination, const char *source, size_t count);

#endif
