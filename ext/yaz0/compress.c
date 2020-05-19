/*
 * Yaz0
 *
 * Compress yaz0 files
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "yaz0.h"

#define RUN_NONE 0
#define RUN_DATA 1
#define RUN_REF 2

typedef struct
{
    uint8_t type;
    union {
        uint8_t data;
        struct
        {
            uint16_t offset;
            uint16_t len;
        };
    };
} Yaz0Run;

static void makeRunNone(Yaz0Run *r)
{
    r->type = RUN_NONE;
}

static void makeRunData(Yaz0Run *r, uint8_t data)
{
    r->type = RUN_DATA;
    r->data = data;
}

static void makeRunRef(Yaz0Run *r, uint16_t offset, uint16_t len)
{
    r->type = RUN_REF;
    r->offset = offset;
    r->len = len;
}

typedef struct
{
    Yaz0Run r[8];
    uint32_t len;
    uint8_t group;
} Yaz0Chunk;

static void makeChunkEmpty(Yaz0Chunk *c)
{
    for (int i = 0; i < 8; ++i)
    {
        makeRunNone(&(c->r[i]));
    }
    c->len = 0;
    c->group = 0;
}

typedef struct
{
    Yaz0Buffer *dst;
    const char *data;
    uint32_t dataSize;
    int32_t inCursor;
} Yaz0Compressor;

static int runCost(const Yaz0Run *run)
{
    switch (run->type)
    {
    case RUN_NONE:
        return 1000;
    case RUN_DATA:
        return 0;
    case RUN_REF:
        return ((run->len >= 0x12) ? 3 : 2) - run->len;
    }
    return 0;
}

static int chunkCost(const Yaz0Chunk *ch)
{
    int acc;

    acc = 0;
    for (int i = 0; i < 8; ++i)
    {
        acc += runCost(ch->r + i);
    }

    return acc;
}

static void bestChunk(Yaz0Chunk *dst, const Yaz0Chunk *a, const Yaz0Chunk *b)
{
    const Yaz0Chunk *src = chunkCost(a) <= chunkCost(b) ? a : b;
    if (src != dst)
    {
        memcpy(dst, src, sizeof(*dst));
    }
}

static void bestRun(Yaz0Run *dst, const Yaz0Run *a, const Yaz0Run *b)
{
    const Yaz0Run *src = (runCost(a) <= runCost(b)) ? a : b;
    if (src != dst)
    {
        memcpy(dst, src, sizeof(*dst));
    }
}

static uint32_t runLength(const Yaz0Run *r)
{
    switch (r->type)
    {
    case RUN_NONE:
        return 0;
    case RUN_DATA:
        return 1;
    case RUN_REF:
        return r->len;
    }
    return 0;
}

static int makeMatchPattern(Yaz0Run *run, Yaz0Compressor *compressor, int len)
{
    const char *pattern = compressor->data + compressor->inCursor;
    int cursorBase = (int)compressor->inCursor - 0x1000;
    int cursorMax = (int)compressor->inCursor - len;

    if (compressor->inCursor + len > compressor->dataSize)
        return 0;

    if (cursorBase < 0)
        cursorBase = 0;

    if (cursorMax < 0)
        return 0;

    for (int i = cursorMax; i >= cursorBase; --i)
    {
        if (memcmp(compressor->data + i, pattern, len) == 0)
        {
            makeRunRef(run, compressor->inCursor - i, len);
            return 1;
        }
    }
    return 0;
}

static void refineMatch(Yaz0Compressor *compressor, Yaz0Run *run)
{
    if (run->type != RUN_REF)
        return;

    if (run->offset != run->len)
        return;

    int initialLen = run->len;

    for (;;)
    {
        if (run->len == 0x111)
            return;
        if (compressor->inCursor + run->len == compressor->dataSize)
            return;
        if (compressor->data[compressor->inCursor + run->len] != compressor->data[compressor->inCursor + (run->len % initialLen)])
            return;
        run->len++;
    }
}

static void makeMatch(Yaz0Run *dst, Yaz0Compressor *compressor)
{
    Yaz0Run run;

    for (int i = 1; i <= 0x111; ++i)
    {
        if (!makeMatchPattern(&run, compressor, i))
            break;
        refineMatch(compressor, &run);
        bestRun(dst, &run, dst);
    }
}

static void makeRun(Yaz0Run *dst, Yaz0Compressor *compressor, int dataOnly)
{
    Yaz0Run tmp;

    makeRunNone(dst);
    if (compressor->inCursor >= compressor->dataSize)
        return;
    /* Get the basic data run */
    makeRunData(dst, compressor->data[compressor->inCursor]);
    if (!dataOnly)
    {
        makeRunNone(&tmp);
        makeMatch(&tmp, compressor);
        bestRun(dst, dst, &tmp);
    }
}

static void writeRun(Yaz0Compressor *c, const Yaz0Run *r)
{
    uint8_t tmp8;
    uint16_t tmp16;

    switch (r->type)
    {
    case RUN_NONE:
        break;
    case RUN_DATA:
        yaz0BufferWrite(c->dst, &r->data, 1);
        break;
    case RUN_REF:
        tmp16 = (r->offset - 1);
        if (r->len <= 0x11)
        {
            tmp16 |= ((((uint16_t)r->len) - 2) << 12);
            tmp16 = swap16(tmp16);
            yaz0BufferWrite(c->dst, &tmp16, 2);
        }
        else
        {
            tmp16 = swap16(tmp16);
            yaz0BufferWrite(c->dst, &tmp16, 2);
            tmp8 = (r->len - 0x12);
            yaz0BufferWrite(c->dst, &tmp8, 1);
        }
        break;
    }
}

static void makeChunkCandidate(Yaz0Chunk *dst, Yaz0Compressor *c, uint8_t mask)
{
    uint32_t cursor;

    cursor = c->inCursor;
    makeChunkEmpty(dst);
    for (int i = 0; i < 8; ++i)
    {
        makeRun(dst->r + i, c, mask & (1 << (7 - i)));
        if (dst->r[i].type == RUN_DATA)
        {
            dst->group |= (1 << (7 - i));
        }
        c->inCursor += runLength(dst->r + i);
        dst->len += runLength(dst->r + i);
    }

    c->inCursor = cursor;
}

static void makeChunk(Yaz0Compressor *c)
{
    Yaz0Chunk ch;
    Yaz0Chunk ch2;

    makeChunkCandidate(&ch, c, 0x00);
    makeChunkCandidate(&ch2, c, 0x01);
    bestChunk(&ch, &ch, &ch2);
    makeChunkCandidate(&ch2, c, 0x02);
    bestChunk(&ch, &ch, &ch2);
    makeChunkCandidate(&ch2, c, 0x04);
    bestChunk(&ch, &ch, &ch2);
    makeChunkCandidate(&ch2, c, 0x08);
    bestChunk(&ch, &ch, &ch2);
    makeChunkCandidate(&ch2, c, 0x10);
    bestChunk(&ch, &ch, &ch2);
    makeChunkCandidate(&ch2, c, 0x20);
    bestChunk(&ch, &ch, &ch2);
    makeChunkCandidate(&ch2, c, 0x40);
    bestChunk(&ch, &ch, &ch2);
    makeChunkCandidate(&ch2, c, 0x80);
    bestChunk(&ch, &ch, &ch2);

    yaz0BufferWrite(c->dst, &ch.group, 1);
    for (int i = 0; i < 8; ++i)
    {
        writeRun(c, ch.r + i);
    }
    c->inCursor += ch.len;
}

static void writeHeader(Yaz0Compressor *c)
{
    uint32_t tmp;

    yaz0BufferWrite(c->dst, "Yaz0", 4);
    tmp = swap32(c->dataSize);
    yaz0BufferWrite(c->dst, &tmp, 4);
    tmp = 0;
    yaz0BufferWrite(c->dst, &tmp, 4);
    yaz0BufferWrite(c->dst, &tmp, 4);
}

int yaz0Compress(Yaz0Buffer *dst, const char *data, size_t len)
{
    Yaz0Compressor compressor;

    compressor.dst = dst;
    compressor.data = data;
    compressor.dataSize = len;
    compressor.inCursor = 0;

    writeHeader(&compressor);

    while (compressor.inCursor < compressor.dataSize)
        makeChunk(&compressor);
    return 1;
}
