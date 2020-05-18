#ifndef YAZ0_H
#define YAZ0_H 1

#include <stddef.h>
#include "ruby.h"

/*
 * Swap
 */
inline static uint16_t swap16(uint16_t v)
{
    return (v << 8) | (v >> 8);
}

inline static uint32_t swap32(uint32_t v)
{
    return (v << 24) | ((v << 8) & 0x00ff0000) | ((v >> 8) & 0x0000ff00) | (v >> 24);
}

/*
 * Buffer
 */

typedef struct
{
    size_t size;
    size_t capacity;
    char *data;
} Yaz0Buffer;

void yaz0BufferAlloc(Yaz0Buffer *buf, size_t cap);
void yaz0BufferFree(Yaz0Buffer *buf);
void yaz0BufferWrite(Yaz0Buffer *buf, const void *data, size_t len);

int yaz0Compress(Yaz0Buffer *dst, const char *data, size_t len);
int yaz0Decompress(Yaz0Buffer *dst, const char *data, size_t len);

#endif /* YAZ0_H */
