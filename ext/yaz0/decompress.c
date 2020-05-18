#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "yaz0.h"

int yaz0Decompress(Yaz0Buffer *dst, const char *d, size_t dataLen)
{
    const uint8_t *data;
    uint32_t dstSize;
    unsigned inCursor;
    uint8_t group;
    int len;
    int rrr;

    data = d;
    if (dataLen < 0x10)
        return 0;
    if (memcmp(data, "Yaz0", 4) != 0)
        return 0;

    dstSize = *(uint32_t *)(data + 4);
    dstSize = swap32(dstSize);

    inCursor = 0x10;
    dst->data = realloc(dst->data, dstSize);
    dst->capacity = dstSize;

    while (inCursor < dataLen && dst->size < dstSize)
    {
        group = data[inCursor++];
        for (int b = 0; b < 8; ++b)
        {
            if (inCursor >= dataLen || dst->size >= dstSize)
                break;

            if (group & (1 << (7 - b)))
            {
                /* Direct data */
                dst->data[dst->size++] = data[inCursor++];
            }
            else
            {
                if (data[inCursor] & 0xf0)
                {
                    len = (data[inCursor] >> 4) + 2;
                    rrr = (((data[inCursor] & 0xf) << 8) | (data[inCursor + 1])) + 1;
                    inCursor += 2;
                }
                else
                {
                    rrr = (((data[inCursor] & 0xf) << 8) | (data[inCursor + 1])) + 1;
                    len = (data[inCursor + 2]) + 0x12;
                    inCursor += 3;
                }
                for (int i = 0; i < len; ++i)
                {
                    dst->data[dst->size] = dst->data[dst->size - rrr];
                    dst->size++;
                }
            }
        }
    }

    return 1;
}
