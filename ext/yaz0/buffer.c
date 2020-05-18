#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "yaz0.h"

void yaz0BufferAlloc(Yaz0Buffer *buf, size_t cap)
{
    buf->size = 0;
    buf->capacity = cap;
    buf->data = malloc(cap);
}

void yaz0BufferFree(Yaz0Buffer *buf)
{
    free(buf->data);
    buf->data = NULL;
}

void yaz0BufferWrite(Yaz0Buffer *buf, const void *data, size_t len)
{
    while (buf->size + len > buf->capacity)
    {
        buf->capacity = buf->capacity + buf->capacity / 2;
        buf->data = realloc(buf->data, buf->capacity);
    }

    memcpy(buf->data + buf->size, data, len);
    buf->size += len;
}
