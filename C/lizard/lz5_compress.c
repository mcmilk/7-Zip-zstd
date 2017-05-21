/*
    LZ5 - Fast LZ compression algorithm 
    Copyright (C) 2011-2015, Yann Collet.
    Copyright (C) 2015-2016, Przemyslaw Skibinski <inikep@gmail.com>

    BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOTLZ5_hash4Ptr
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
       - LZ5 source repository : https://github.com/inikep/lz5
*/


/* *************************************
*  Includes
***************************************/
#include "lz5_compress.h"
#include "lz5_common.h"
#include <stdio.h>
#include <stdint.h> // intptr_t
#ifndef USE_LZ4_ONLY
    #ifdef LZ5_USE_TEST
        #include "test/lz5_common_test.h"
        #include "test/lz5_compress_test.h"
    #else
        #include "lz5_compress_lz5v2.h"
    #endif
#endif
#include "lz5_compress_lz4.h"
#include "entropy/huf.h"


/* *************************************
*  Local Macros
***************************************/
#define DELTANEXT(p)        chainTable[(p) & contentMask]
#define LZ5_MINIMAL_HUFF_GAIN(comprSize) (comprSize + (comprSize/8) + 512)
#define LZ5_MINIMAL_BLOCK_GAIN(comprSize) (comprSize + (comprSize/32) + 512)


/*-************************************
*  Local Utils
**************************************/
int LZ5_versionNumber (void) { return LZ5_VERSION_NUMBER; }
int LZ5_compressBound(int isize)  { return LZ5_COMPRESSBOUND(isize); }
int LZ5_sizeofState_MinLevel() { return LZ5_sizeofState(LZ5_MIN_CLEVEL); }



/* *************************************
*  Hash functions
***************************************/
#define HASH_UPDATE_LIMIT 8  /* equal to MEM_read64 */
static const U32 prime4bytes = 2654435761U;
static const U64 prime5bytes = 889523592379ULL;
static const U64 prime6bytes = 227718039650203ULL;
static const U64 prime7bytes = 58295818150454627ULL;

#if MINMATCH == 3
static const U32 prime3bytes = 506832829U;
static U32 LZ5_hash3(U32 u, U32 h) { return (u * prime3bytes) << (32-24) >> (32-h) ; }
static size_t LZ5_hash3Ptr(const void* ptr, U32 h) { return LZ5_hash3(MEM_read32(ptr), h); }
#endif

static U32 LZ5_hash4(U32 u, U32 h) { return (u * prime4bytes) >> (32-h) ; }
static size_t LZ5_hash4Ptr(const void* ptr, U32 h) { return LZ5_hash4(MEM_read32(ptr), h); }

static size_t LZ5_hash5(U64 u, U32 h) { return (size_t)((u * prime5bytes) << (64-40) >> (64-h)) ; }
static size_t LZ5_hash5Ptr(const void* p, U32 h) { return LZ5_hash5(MEM_read64(p), h); }

static size_t LZ5_hash6(U64 u, U32 h) { return (size_t)((u * prime6bytes) << (64-48) >> (64-h)) ; }
static size_t LZ5_hash6Ptr(const void* p, U32 h) { return LZ5_hash6(MEM_read64(p), h); }

static size_t LZ5_hash7(U64 u, U32 h) { return (size_t)((u * prime7bytes) << (64-56) >> (64-h)) ; }
static size_t LZ5_hash7Ptr(const void* p, U32 h) { return LZ5_hash7(MEM_read64(p), h); }

static size_t LZ5_hashPtr(const void* p, U32 hBits, U32 mls)
{
    switch(mls)
    {
    default:
    case 4: return LZ5_hash4Ptr(p, hBits);
    case 5: return LZ5_hash5Ptr(p, hBits);
    case 6: return LZ5_hash6Ptr(p, hBits);
    case 7: return LZ5_hash7Ptr(p, hBits);
    }
} 




/**************************************
*  Internal functions
**************************************/
/** LZ5_count_2segments() :
*   can count match length with `ip` & `match` in 2 different segments.
*   convention : on reaching mEnd, match count continue starting from iStart
*/
static size_t LZ5_count_2segments(const BYTE* ip, const BYTE* match, const BYTE* iEnd, const BYTE* mEnd, const BYTE* iStart)
{
    const BYTE* const vEnd = MIN( ip + (mEnd - match), iEnd);
    size_t const matchLength = LZ5_count(ip, match, vEnd);
    if (match + matchLength != mEnd) return matchLength;
    return matchLength + LZ5_count(ip+matchLength, iStart, iEnd);
}


void LZ5_initBlock(LZ5_stream_t* ctx)
{
    ctx->offset16Ptr = ctx->offset16Base;
    ctx->offset24Ptr = ctx->offset24Base;
    ctx->lenPtr      = ctx->lenBase;
    ctx->literalsPtr = ctx->literalsBase;
    ctx->flagsPtr = ctx->flagsBase;
    ctx->last_off = LZ5_INIT_LAST_OFFSET; /* reset last offset */
}


FORCE_INLINE int LZ5_writeStream(int useHuff, LZ5_stream_t* ctx, BYTE* streamPtr, uint32_t streamLen, BYTE** op, BYTE* oend)
{
    if (useHuff && streamLen > 1024) {
#ifndef LZ5_NO_HUFFMAN
        int useHuffBuf;
        if (*op + 6 > oend) { LZ5_LOG_COMPRESS("*op[%p] + 6 > oend[%p]\n", *op, oend); return -1; }

        useHuffBuf = ((size_t)(oend - (*op + 6)) < HUF_compressBound(streamLen)); 
        if (useHuffBuf) {
            if (streamLen > LZ5_BLOCK_SIZE) { LZ5_LOG_COMPRESS("streamLen[%d] > LZ5_BLOCK_SIZE\n", streamLen); return -1; }
            ctx->comprStreamLen = (U32)HUF_compress(ctx->huffBase, ctx->huffEnd - ctx->huffBase, streamPtr, streamLen);
        } else {
            ctx->comprStreamLen = (U32)HUF_compress(*op + 6, oend - (*op + 6), streamPtr, streamLen);
        }

        if (!HUF_isError(ctx->comprStreamLen)) {
            if (ctx->comprStreamLen > 0 && (LZ5_MINIMAL_HUFF_GAIN(ctx->comprStreamLen) < streamLen)) { /* compressible */
                MEM_writeLE24(*op, streamLen);
                MEM_writeLE24(*op+3, ctx->comprStreamLen);
                if (useHuffBuf) {
                    if ((size_t)(oend - (*op + 6)) < ctx->comprStreamLen) { LZ5_LOG_COMPRESS("*op[%p] oend[%p] comprStreamLen[%d]\n", *op, oend, (int)ctx->comprStreamLen); return -1; }
                    memcpy(*op + 6, ctx->huffBase, ctx->comprStreamLen);
                }
                *op += ctx->comprStreamLen + 6;
                LZ5_LOG_COMPRESS("HUF_compress streamLen=%d comprStreamLen=%d\n", (int)streamLen, (int)ctx->comprStreamLen);
                return 1;
            } else { LZ5_LOG_COMPRESS("HUF_compress ERROR comprStreamLen=%d streamLen=%d\n", (int)ctx->comprStreamLen, (int)streamLen); }
        } else { LZ5_LOG_COMPRESS("HUF_compress ERROR %d: %s\n", (int)ctx->comprStreamLen, HUF_getErrorName(ctx->comprStreamLen)); }
#else
        LZ5_LOG_COMPRESS("compiled with LZ5_NO_HUFFMAN\n");
        (void)ctx;
        return -1; 
#endif
    } else ctx->comprStreamLen = 0;

    if (*op + 3 + streamLen > oend) { LZ5_LOG_COMPRESS("*op[%p] + 3 + streamLen[%d] > oend[%p]\n", *op, streamLen, oend); return -1; }
    MEM_writeLE24(*op, streamLen);
    *op += 3;
    memcpy(*op, streamPtr, streamLen);
    *op += streamLen;
    LZ5_LOG_COMPRESS("Uncompressed streamLen=%d\n", (int)streamLen);
    return 0;
}


int LZ5_writeBlock(LZ5_stream_t* ctx, const BYTE* ip, uint32_t inputSize, BYTE** op, BYTE* oend)
{
    int res;
    uint32_t flagsLen = (uint32_t)(ctx->flagsPtr - ctx->flagsBase);
    uint32_t literalsLen = (uint32_t)(ctx->literalsPtr - ctx->literalsBase);
    uint32_t lenLen = (uint32_t)(ctx->lenPtr - ctx->lenBase);
    uint32_t offset16Len = (uint32_t)(ctx->offset16Ptr - ctx->offset16Base);
    uint32_t offset24Len = (uint32_t)(ctx->offset24Ptr - ctx->offset24Base);
    uint32_t sum = flagsLen + literalsLen + lenLen + offset16Len + offset24Len;
#ifdef LZ5_USE_LOGS
    uint32_t comprFlagsLen, comprLiteralsLen;
#endif

    BYTE* start = *op;
    
    if ((literalsLen < WILDCOPYLENGTH) || (sum+5*3+1 > inputSize)) goto _write_uncompressed;

    *start = 0;
    *op += 1;

    res = LZ5_writeStream(0, ctx, ctx->lenBase, lenLen, op, oend);
    if (res < 0) goto _output_error; else *start += (BYTE)(res*LZ5_FLAG_LEN);

    res = LZ5_writeStream(ctx->huffType&LZ5_FLAG_OFFSET16, ctx, ctx->offset16Base, offset16Len, op, oend);
    if (res < 0) goto _output_error; else *start += (BYTE)(res*LZ5_FLAG_OFFSET16);

    res = LZ5_writeStream(ctx->huffType&LZ5_FLAG_OFFSET24, ctx, ctx->offset24Base, offset24Len, op, oend);
    if (res < 0) goto _output_error; else *start += (BYTE)(res*LZ5_FLAG_OFFSET24);

    res = LZ5_writeStream(ctx->huffType&LZ5_FLAG_FLAGS, ctx, ctx->flagsBase, flagsLen, op, oend);
    if (res < 0) goto _output_error; else *start += (BYTE)(res*LZ5_FLAG_FLAGS);
#ifdef LZ5_USE_LOGS
    comprFlagsLen = ctx->comprStreamLen;
#endif

    res = LZ5_writeStream(ctx->huffType&LZ5_FLAG_LITERALS, ctx, ctx->literalsBase, literalsLen, op, oend);
    if (res < 0) goto _output_error; else *start += (BYTE)(res*LZ5_FLAG_LITERALS);
#ifdef LZ5_USE_LOGS
    comprLiteralsLen = ctx->comprStreamLen;
    sum = (int)(*op-start);
#endif

    if (LZ5_MINIMAL_BLOCK_GAIN((uint32_t)(*op-start)) > inputSize) goto _write_uncompressed;

    LZ5_LOG_COMPRESS("%d: total=%d block=%d flagsLen[%.2f%%]=%d comprFlagsLen[%.2f%%]=%d literalsLen[%.2f%%]=%d comprLiteralsLen[%.2f%%]=%d lenLen=%d offset16Len[%.2f%%]=%d offset24Len[%.2f%%]=%d\n", (int)(ip - ctx->srcBase),
            (int)(*op - ctx->destBase), sum, (flagsLen*100.0)/sum, flagsLen, (comprFlagsLen*100.0)/sum, comprFlagsLen, (literalsLen*100.0)/sum, literalsLen, (comprLiteralsLen*100.0)/sum, comprLiteralsLen,
            lenLen, (offset16Len*100.0)/sum, offset16Len, (offset24Len*100.0)/sum, offset24Len);
    return 0;

_write_uncompressed:
    LZ5_LOG_COMPRESS("%d: total=%d block=%d UNCOMPRESSED inputSize=%u outSize=%d\n", (int)(ip - ctx->srcBase),
            (int)(*op - ctx->destBase), (int)(*op-start), inputSize, (int)(oend-start));
    if ((uint32_t)(oend - start) < inputSize + 4) goto _output_error;
    *start = LZ5_FLAG_UNCOMPRESSED;
    *op = start + 1;
    MEM_writeLE24(*op, inputSize);
    *op += 3;
    memcpy(*op, ip, inputSize);
    *op += inputSize;
    return 0;

_output_error:
    LZ5_LOG_COMPRESS("LZ5_writeBlock ERROR size=%d/%d flagsLen=%d literalsLen=%d lenLen=%d offset16Len=%d offset24Len=%d\n", (int)(*op-start), (int)(oend-start), flagsLen, literalsLen, lenLen, offset16Len, offset24Len);
    return 1;
}


FORCE_INLINE int LZ5_encodeSequence (
    LZ5_stream_t* ctx,
    const BYTE** ip,
    const BYTE** anchor,
    size_t matchLength,
    const BYTE* const match)
{
#ifdef USE_LZ4_ONLY
    return LZ5_encodeSequence_LZ4(ctx, ip, anchor, matchLength, match);
#else
    if (ctx->params.decompressType == LZ5_coderwords_LZ4)
        return LZ5_encodeSequence_LZ4(ctx, ip, anchor, matchLength, match);

    return LZ5_encodeSequence_LZ5v2(ctx, ip, anchor, matchLength, match);
#endif
}


FORCE_INLINE int LZ5_encodeLastLiterals (
    LZ5_stream_t* ctx,
    const BYTE** ip,
    const BYTE** anchor)
{
    LZ5_LOG_COMPRESS("LZ5_encodeLastLiterals LZ5_coderwords_LZ4=%d\n", ctx->params.decompressType == LZ5_coderwords_LZ4);    
#ifdef USE_LZ4_ONLY
    return LZ5_encodeLastLiterals_LZ4(ctx, ip, anchor);
#else
    if (ctx->params.decompressType == LZ5_coderwords_LZ4)
        return LZ5_encodeLastLiterals_LZ4(ctx, ip, anchor);

    return LZ5_encodeLastLiterals_LZ5v2(ctx, ip, anchor);
#endif
}


/**************************************
*  Include parsers
**************************************/
#include "lz5_parser_hashchain.h"
#include "lz5_parser_nochain.h"
#include "lz5_parser_fast.h"
#include "lz5_parser_fastsmall.h"
#include "lz5_parser_fastbig.h"
#ifndef USE_LZ4_ONLY
    #include "lz5_parser_optimal.h"
    #include "lz5_parser_lowestprice.h"
    #include "lz5_parser_pricefast.h"
#endif


int LZ5_verifyCompressionLevel(int compressionLevel)
{
    (void)LZ5_hashPtr;
    (void)LZ5_wildCopy16;
    if (compressionLevel > LZ5_MAX_CLEVEL) compressionLevel = LZ5_MAX_CLEVEL;
    if (compressionLevel < LZ5_MIN_CLEVEL) compressionLevel = LZ5_DEFAULT_CLEVEL;
    return compressionLevel;
}


int LZ5_sizeofState(int compressionLevel) 
{ 
    LZ5_parameters params;
    U32 hashTableSize, chainTableSize;

    compressionLevel = LZ5_verifyCompressionLevel(compressionLevel);
    params = LZ5_defaultParameters[compressionLevel - LZ5_MIN_CLEVEL];
//    hashTableSize = (U32)(sizeof(U32)*(((size_t)1 << params.hashLog3)+((size_t)1 << params.hashLog)));
    hashTableSize = (U32)(sizeof(U32)*(((size_t)1 << params.hashLog)));
    chainTableSize = (U32)(sizeof(U32)*((size_t)1 << params.contentLog));

    return sizeof(LZ5_stream_t) + hashTableSize + chainTableSize + LZ5_COMPRESS_ADD_BUF + (int)LZ5_COMPRESS_ADD_HUF;
}


static void LZ5_init(LZ5_stream_t* ctx, const BYTE* start)
{
    MEM_INIT((void*)ctx->hashTable, 0, ctx->hashTableSize);
    MEM_INIT(ctx->chainTable, 0x01, ctx->chainTableSize);
 //   printf("memset hashTable=%p hashEnd=%p chainTable=%p chainEnd=%p\n", ctx->hashTable, ((BYTE*)ctx->hashTable) + ctx->hashTableSize, ctx->chainTable, ((BYTE*)ctx->chainTable)+ctx->chainTableSize);
    ctx->nextToUpdate = LZ5_DICT_SIZE;
    ctx->base = start - LZ5_DICT_SIZE;
    ctx->end = start;
    ctx->dictBase = start - LZ5_DICT_SIZE;
    ctx->dictLimit = LZ5_DICT_SIZE;
    ctx->lowLimit = LZ5_DICT_SIZE;
    ctx->last_off = LZ5_INIT_LAST_OFFSET;
    ctx->litSum = 0;
}


/* if ctx==NULL memory is allocated and returned as value */
LZ5_stream_t* LZ5_initStream(LZ5_stream_t* ctx, int compressionLevel) 
{ 
    LZ5_parameters params;
    U32 hashTableSize, chainTableSize;
    void *tempPtr;

    compressionLevel = LZ5_verifyCompressionLevel(compressionLevel);
    params = LZ5_defaultParameters[compressionLevel - LZ5_MIN_CLEVEL];
//    hashTableSize = (U32)(sizeof(U32)*(((size_t)1 << params.hashLog3)+((size_t)1 << params.hashLog)));
    hashTableSize = (U32)(sizeof(U32)*(((size_t)1 << params.hashLog)));
    chainTableSize = (U32)(sizeof(U32)*((size_t)1 << params.contentLog));
    
    if (!ctx)
    {
        ctx = (LZ5_stream_t*)malloc(sizeof(LZ5_stream_t) + hashTableSize + chainTableSize + LZ5_COMPRESS_ADD_BUF + LZ5_COMPRESS_ADD_HUF);
        if (!ctx) { printf("ERROR: Cannot allocate %d MB (compressionLevel=%d)\n", (int)(sizeof(LZ5_stream_t) + hashTableSize + chainTableSize)>>20, compressionLevel); return 0; }
        LZ5_LOG_COMPRESS("Allocated %d MB (compressionLevel=%d)\n", (int)(sizeof(LZ5_stream_t) + hashTableSize + chainTableSize)>>20, compressionLevel); 
        ctx->allocatedMemory = sizeof(LZ5_stream_t) + hashTableSize + chainTableSize + LZ5_COMPRESS_ADD_BUF + (U32)LZ5_COMPRESS_ADD_HUF;
      //  printf("malloc from=%p to=%p hashTable=%p hashEnd=%p chainTable=%p chainEnd=%p\n", ctx, ((BYTE*)ctx)+sizeof(LZ5_stream_t) + hashTableSize + chainTableSize, ctx->hashTable, ((BYTE*)ctx->hashTable) + hashTableSize, ctx->chainTable, ((BYTE*)ctx->chainTable)+chainTableSize);
    }
    
    tempPtr = ctx;
    ctx->hashTable = (U32*)(tempPtr) + sizeof(LZ5_stream_t)/4;
    ctx->hashTableSize = hashTableSize;
    ctx->chainTable = ctx->hashTable + hashTableSize/4;
    ctx->chainTableSize = chainTableSize;
    ctx->params = params;
    ctx->compressionLevel = (unsigned)compressionLevel;
    if (compressionLevel < 30)
        ctx->huffType = 0;
    else
        ctx->huffType = LZ5_FLAG_LITERALS + LZ5_FLAG_FLAGS; // + LZ5_FLAG_OFFSET16 + LZ5_FLAG_OFFSET24;

    ctx->literalsBase = (BYTE*)ctx->hashTable + ctx->hashTableSize + ctx->chainTableSize;
    ctx->flagsBase    = ctx->literalsEnd = ctx->literalsBase + LZ5_BLOCK_SIZE_PAD;
    ctx->lenBase      = ctx->flagsEnd    = ctx->flagsBase    + LZ5_BLOCK_SIZE_PAD;
    ctx->offset16Base = ctx->lenEnd      = ctx->lenBase      + LZ5_BLOCK_SIZE_PAD;
    ctx->offset24Base = ctx->offset16End = ctx->offset16Base + LZ5_BLOCK_SIZE_PAD;
    ctx->huffBase     = ctx->offset24End = ctx->offset24Base + LZ5_BLOCK_SIZE_PAD;
                        ctx->huffEnd     = ctx->huffBase     + LZ5_COMPRESS_ADD_HUF;

    return ctx;
}



LZ5_stream_t* LZ5_createStream(int compressionLevel) 
{ 
    LZ5_stream_t* ctx = LZ5_initStream(NULL, compressionLevel);
//    if (ctx) printf("LZ5_createStream ctx=%p ctx->compressionLevel=%d\n", ctx, ctx->compressionLevel);
    return ctx; 
}


/* initialization */
LZ5_stream_t* LZ5_resetStream(LZ5_stream_t* ctx, int compressionLevel)
{
    size_t wanted = LZ5_sizeofState(compressionLevel);

//    printf("LZ5_resetStream ctx=%p cLevel=%d have=%d wanted=%d min=%d\n", ctx, compressionLevel, (int)have, (int)wanted, (int)sizeof(LZ5_stream_t));
    if (ctx->allocatedMemory < wanted)
    {
  //      printf("REALLOC ctx=%p cLevel=%d have=%d wanted=%d\n", ctx, compressionLevel, (int)have, (int)wanted);
        LZ5_freeStream(ctx);
        ctx = LZ5_createStream(compressionLevel);
    }
    else
    {
        LZ5_initStream(ctx, compressionLevel);
    }

    if (ctx) ctx->base = NULL;
    return ctx;
}


int LZ5_freeStream(LZ5_stream_t* ctx) 
{ 
    if (ctx) {
     //   printf("LZ5_freeStream ctx=%p ctx->compressionLevel=%d\n", ctx, ctx->compressionLevel);
        free(ctx);
    }
    return 0; 
}


int LZ5_loadDict(LZ5_stream_t* LZ5_streamPtr, const char* dictionary, int dictSize)
{
    LZ5_stream_t* ctxPtr = (LZ5_stream_t*) LZ5_streamPtr;
    if (dictSize > LZ5_DICT_SIZE) {
        dictionary += dictSize - LZ5_DICT_SIZE;
        dictSize = LZ5_DICT_SIZE;
    }
    LZ5_init (ctxPtr, (const BYTE*)dictionary);
    if (dictSize >= HASH_UPDATE_LIMIT) LZ5_Insert (ctxPtr, (const BYTE*)dictionary + (dictSize - (HASH_UPDATE_LIMIT-1)));
    ctxPtr->end = (const BYTE*)dictionary + dictSize;
    return dictSize;
}


static void LZ5_setExternalDict(LZ5_stream_t* ctxPtr, const BYTE* newBlock)
{
    if (ctxPtr->end >= ctxPtr->base + HASH_UPDATE_LIMIT) LZ5_Insert (ctxPtr, ctxPtr->end - (HASH_UPDATE_LIMIT-1));   /* Referencing remaining dictionary content */
    /* Only one memory segment for extDict, so any previous extDict is lost at this stage */
    ctxPtr->lowLimit  = ctxPtr->dictLimit;
    ctxPtr->dictLimit = (U32)(ctxPtr->end - ctxPtr->base);
    ctxPtr->dictBase  = ctxPtr->base;
    ctxPtr->base = newBlock - ctxPtr->dictLimit;
    ctxPtr->end  = newBlock;
    ctxPtr->nextToUpdate = ctxPtr->dictLimit;   /* match referencing will resume from there */
}


/* dictionary saving */
int LZ5_saveDict (LZ5_stream_t* LZ5_streamPtr, char* safeBuffer, int dictSize)
{
    LZ5_stream_t* const ctx = (LZ5_stream_t*)LZ5_streamPtr;
    int const prefixSize = (int)(ctx->end - (ctx->base + ctx->dictLimit));
//printf("LZ5_saveDict dictSize=%d prefixSize=%d ctx->dictLimit=%d\n", dictSize, prefixSize, (int)ctx->dictLimit);
    if (dictSize > LZ5_DICT_SIZE) dictSize = LZ5_DICT_SIZE;
    if (dictSize < 4) dictSize = 0;
    if (dictSize > prefixSize) dictSize = prefixSize;
    memmove(safeBuffer, ctx->end - dictSize, dictSize);
    {   U32 const endIndex = (U32)(ctx->end - ctx->base);
        ctx->end = (const BYTE*)safeBuffer + dictSize;
        ctx->base = ctx->end - endIndex;
        ctx->dictLimit = endIndex - dictSize;
        ctx->lowLimit = endIndex - dictSize;
        if (ctx->nextToUpdate < ctx->dictLimit) ctx->nextToUpdate = ctx->dictLimit;
    }
//printf("2LZ5_saveDict dictSize=%d\n", dictSize);
    return dictSize;
}

FORCE_INLINE int LZ5_compress_generic (
    void* ctxvoid,
    const char* source,
    char* dest,
    int inputSize,
    int maxOutputSize)
{
    LZ5_stream_t* ctx = (LZ5_stream_t*) ctxvoid;
    size_t dictSize = (size_t)(ctx->end - ctx->base) - ctx->dictLimit;
    const BYTE* ip = (const BYTE*) source;
    BYTE* op = (BYTE*) dest;
    BYTE* const oend = op + maxOutputSize;
    int res;

    (void)dictSize;
    LZ5_LOG_COMPRESS("LZ5_compress_generic source=%p inputSize=%d dest=%p maxOutputSize=%d cLevel=%d dictBase=%p dictSize=%d\n", source, inputSize, dest, maxOutputSize, ctx->compressionLevel, ctx->dictBase, (int)dictSize); 
    *op++ = (BYTE)ctx->compressionLevel;
    maxOutputSize--; // can be lower than 0
    ctx->end += inputSize;
    ctx->srcBase = ctx->off24pos = ip;
    ctx->destBase = (BYTE*)dest;

    while (inputSize > 0)
    {
        int inputPart = MIN(LZ5_BLOCK_SIZE, inputSize);

        if (ctx->huffType) LZ5_rescaleFreqs(ctx);
        LZ5_initBlock(ctx);
        ctx->diffBase = ip;

        switch(ctx->params.parserType)
        {
        default:
        case LZ5_parser_fastSmall:
            res = LZ5_compress_fastSmall(ctx, ip, ip+inputPart); break;
        case LZ5_parser_fast:
            res = LZ5_compress_fast(ctx, ip, ip+inputPart); break;
        case LZ5_parser_noChain:
            res = LZ5_compress_noChain(ctx, ip, ip+inputPart); break;
        case LZ5_parser_hashChain:
            res = LZ5_compress_hashChain(ctx, ip, ip+inputPart); break;
#ifndef USE_LZ4_ONLY
        case LZ5_parser_fastBig:
            res = LZ5_compress_fastBig(ctx, ip, ip+inputPart); break;
        case LZ5_parser_priceFast:
            res = LZ5_compress_priceFast(ctx, ip, ip+inputPart); break;
        case LZ5_parser_lowestPrice:
            res = LZ5_compress_lowestPrice(ctx, ip, ip+inputPart); break;
        case LZ5_parser_optimalPrice:
        case LZ5_parser_optimalPriceBT:
            res = LZ5_compress_optimalPrice(ctx, ip, ip+inputPart); break;
#else
        case LZ5_parser_priceFast:
        case LZ5_parser_lowestPrice:
        case LZ5_parser_optimalPrice:
        case LZ5_parser_optimalPriceBT:
            res = 0;
#endif
        }

        LZ5_LOG_COMPRESS("LZ5_compress_generic res=%d inputPart=%d \n", res, inputPart);
        if (res <= 0) return res;

        if (LZ5_writeBlock(ctx, ip, inputPart, &op, oend)) goto _output_error;

        ip += inputPart;
        inputSize -= inputPart;
        LZ5_LOG_COMPRESS("LZ5_compress_generic in=%d out=%d\n", (int)(ip-(const BYTE*)source), (int)(op-(BYTE*)dest));
    }

    LZ5_LOG_COMPRESS("LZ5_compress_generic total=%d\n", (int)(op-(BYTE*)dest));
    return (int)(op-(BYTE*)dest);
_output_error:
    LZ5_LOG_COMPRESS("LZ5_compress_generic ERROR\n");
    return 0;
}


int LZ5_compress_continue (LZ5_stream_t* ctxPtr,
                                            const char* source, char* dest,
                                            int inputSize, int maxOutputSize)
{
    /* auto-init if forgotten */
    if (ctxPtr->base == NULL) LZ5_init (ctxPtr, (const BYTE*) source);

    /* Check overflow */
    if ((size_t)(ctxPtr->end - ctxPtr->base) > 2 GB) {
        size_t dictSize = (size_t)(ctxPtr->end - ctxPtr->base) - ctxPtr->dictLimit;
        if (dictSize > LZ5_DICT_SIZE) dictSize = LZ5_DICT_SIZE;
        LZ5_loadDict((LZ5_stream_t*)ctxPtr, (const char*)(ctxPtr->end) - dictSize, (int)dictSize);
    }

    /* Check if blocks follow each other */
    if ((const BYTE*)source != ctxPtr->end) 
        LZ5_setExternalDict(ctxPtr, (const BYTE*)source);

    /* Check overlapping input/dictionary space */
    {   const BYTE* sourceEnd = (const BYTE*) source + inputSize;
        const BYTE* const dictBegin = ctxPtr->dictBase + ctxPtr->lowLimit;
        const BYTE* const dictEnd   = ctxPtr->dictBase + ctxPtr->dictLimit;
        if ((sourceEnd > dictBegin) && ((const BYTE*)source < dictEnd)) {
            if (sourceEnd > dictEnd) sourceEnd = dictEnd;
            ctxPtr->lowLimit = (U32)(sourceEnd - ctxPtr->dictBase);
            if (ctxPtr->dictLimit - ctxPtr->lowLimit < 4) ctxPtr->lowLimit = ctxPtr->dictLimit;
        }
    }

    return LZ5_compress_generic (ctxPtr, source, dest, inputSize, maxOutputSize);
}


int LZ5_compress_extState (void* state, const char* src, char* dst, int srcSize, int maxDstSize, int compressionLevel)
{
    LZ5_stream_t* ctx = (LZ5_stream_t*) state;
    if (((size_t)(state)&(sizeof(void*)-1)) != 0) return 0;   /* Error : state is not aligned for pointers (32 or 64 bits) */

    /* initialize stream */
    LZ5_initStream(ctx, compressionLevel);
    LZ5_init ((LZ5_stream_t*)state, (const BYTE*)src);

    return LZ5_compress_generic (state, src, dst, srcSize, maxDstSize);
}


int LZ5_compress(const char* src, char* dst, int srcSize, int maxDstSize, int compressionLevel)
{
    int cSize;
    LZ5_stream_t* statePtr = LZ5_createStream(compressionLevel);

    if (!statePtr) return 0;
    cSize = LZ5_compress_extState(statePtr, src, dst, srcSize, maxDstSize, compressionLevel);

    LZ5_freeStream(statePtr);
    return cSize;
}


/**************************************
*  Level1 functions
**************************************/
int LZ5_compress_extState_MinLevel(void* state, const char* source, char* dest, int inputSize, int maxOutputSize)
{
    return LZ5_compress_extState(state, source, dest, inputSize, maxOutputSize, LZ5_MIN_CLEVEL);
}

int LZ5_compress_MinLevel(const char* source, char* dest, int inputSize, int maxOutputSize)
{
    return LZ5_compress(source, dest, inputSize, maxOutputSize, LZ5_MIN_CLEVEL);
}

LZ5_stream_t* LZ5_createStream_MinLevel(void)
{
    return LZ5_createStream(LZ5_MIN_CLEVEL);
}

LZ5_stream_t* LZ5_resetStream_MinLevel(LZ5_stream_t* LZ5_stream)
{
    return LZ5_resetStream (LZ5_stream, LZ5_MIN_CLEVEL);
}
