#include "clipplus_memory.h"

void *clipplus_memset(void *destination, int value, size_t count)
{
    unsigned char *out = (unsigned char *)destination;
    while (count--) *out++ = (unsigned char)value;
    return destination;
}

size_t clipplus_strlen(const char *text)
{
    const char *cursor = text;
    while (*cursor) ++cursor;
    return (size_t)(cursor - text);
}

const char *clipplus_strrchr(const char *text, int value)
{
    const char *last = 0;
    for (; *text; ++text) if (*text == (char)value) last = text;
    return value == '\0' ? text : last;
}

char *clipplus_strncpy(char *destination, const char *source, size_t count)
{
    char *out = destination;
    while (count && *source) {
        *out++ = *source++;
        --count;
    }
    while (count--) *out++ = '\0';
    return destination;
}
