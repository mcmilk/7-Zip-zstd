/*
* Copyright (c) 2018, Conor McCarthy
* All rights reserved.
* Parts based on zstd_compress.c copyright Yann Collet
*
* This source code is licensed under both the BSD-style license (found in the
* LICENSE file in the root directory of this source tree) and the GPLv2 (found
* in the COPYING file in the root directory of this source tree).
* You may select, at your option, one of the above-listed licenses.
*/

#include <string.h>
#include "fast-lzma2.h"
#include "fl2_internal.h"
#include "platform.h"
#include "mem.h"
#include "util.h"
#include "fl2_compress_internal.h"
#include "fl2threading.h"
#include "fl2pool.h"
#include "radix_mf.h"
#include "lzma2_enc.h"

#define MIN_BYTES_PER_THREAD 0x10000

#define ALIGNMENT_MASK (~(size_t)15)

/*-=====  Pre-defined compression levels  =====-*/

#define FL2_CLEVEL_DEFAULT   9
#define FL2_MAX_CLEVEL      12
#define FL2_MAX_7Z_CLEVEL   9
#define FL2_MAX_HIGH_CLEVEL 9

FL2LIB_API int FL2LIB_CALL FL2_maxCLevel(void)
{ 
    return FL2_MAX_CLEVEL;
}

FL2LIB_API int FL2LIB_CALL FL2_maxHighCLevel(void)
{
    return FL2_MAX_HIGH_CLEVEL;
}

static const FL2_compressionParameters FL2_defaultCParameters[FL2_MAX_CLEVEL + 1] = {
    { 0,0,0,0,0,0,0 },
    { 20, 1, 7, 0, 6, 32, 1, 8, FL2_fast }, /* 1 */
    { 20, 2, 7, 0, 12, 32, 1, 8, FL2_fast }, /* 2 */
    { 21, 2, 7, 0, 14, 32, 1, 8, FL2_fast }, /* 3 */
    { 20, 2, 7, 0, 12, 32, 1, 8, FL2_opt }, /* 4 */
    { 21, 2, 7, 0, 14, 40, 1, 8, FL2_opt }, /* 5 */
    { 22, 2, 7, 0, 26, 40, 1, 8, FL2_opt }, /* 6 */
    { 23, 2, 8, 0, 42, 48, 1, 8, FL2_opt }, /* 7 */
    { 24, 2, 9, 0, 42, 48, 1, 8, FL2_ultra }, /* 8 */
    { 25, 2, 10, 0, 50, 64, 1, 8, FL2_ultra }, /* 9 */
    { 26, 2, 11, 1, 60, 64, 1, 9, FL2_ultra }, /* 10 */
    { 27, 2, 12, 2, 126, 96, 1, 10, FL2_ultra }, /* 11 */
    { 28, 2, 14, 3, 254, 160, 1, 10, FL2_ultra } /* 12 */
};

static const FL2_compressionParameters FL2_7zCParameters[FL2_MAX_7Z_CLEVEL + 1] = {
    { 0,0,0,0,0,0,0 },
    { 20, 1, 7, 0, 6, 32, 1, 8, FL2_fast }, /* 1 */
    { 20, 2, 7, 0, 12, 32, 1, 8, FL2_fast }, /* 2 */
    { 21, 2, 7, 0, 16, 32, 1, 8, FL2_fast }, /* 3 */
    { 20, 2, 7, 0, 16, 32, 1, 8, FL2_opt }, /* 4 */
    { 24, 2, 9, 0, 40, 48, 1, 8, FL2_ultra }, /* 5 */
    { 25, 2, 10, 0, 48, 64, 1, 8, FL2_ultra }, /* 6 */
    { 26, 2, 11, 1, 60, 96, 1, 9, FL2_ultra }, /* 7 */
    { 27, 2, 12, 2, 128, 128, 1, 10, FL2_ultra }, /* 8 */
    { 27, 3, 14, 3, 252, 160, 0, 10, FL2_ultra } /* 9 */
};

static const FL2_compressionParameters FL2_highCParameters[FL2_MAX_HIGH_CLEVEL + 1] = {
    { 0,0,0,0,0,0,0 },
    { 20, 3, 9, 1, 60, 128, 0, 8, FL2_ultra }, /* 1 */
    { 21, 3, 10, 1, 60, 128, 0, 8, FL2_ultra }, /* 2 */
    { 22, 3, 11, 2, 60, 128, 0, 8, FL2_ultra }, /* 3 */
    { 23, 3, 12, 2, 60, 128, 0, 8, FL2_ultra }, /* 4 */
    { 24, 3, 13, 3, 60, 128, 0, 8, FL2_ultra }, /* 5 */
    { 25, 3, 14, 3, 60, 160, 0, 8, FL2_ultra }, /* 6 */
    { 26, 3, 14, 4, 60, 160, 0, 8, FL2_ultra }, /* 7 */
    { 27, 3, 14, 4, 128, 160, 0, 8, FL2_ultra }, /* 8 */
    { 28, 3, 14, 5, 128, 160, 0, 9, FL2_ultra } /* 9 */
};

void FL2_fillParameters(FL2_CCtx* const cctx, const FL2_compressionParameters* const params)
{
    FL2_lzma2Parameters* const cParams = &cctx->params.cParams;
    RMF_parameters* const rParams = &cctx->params.rParams;
    cParams->lc = 3;
    cParams->lp = 0;
    cParams->pb = 2;
    cParams->fast_length = params->fastLength;
    cParams->match_cycles = 1U << params->searchLog;
    cParams->strategy = params->strategy;
    cParams->second_dict_bits = params->chainLog;
    cParams->random_filter = 0;
    rParams->dictionary_log = MIN(params->dictionaryLog, FL2_DICTLOG_MAX); /* allow for reduced dict in 32-bit version */
    rParams->match_buffer_log = params->bufferLog;
    rParams->overlap_fraction = params->overlapFraction;
    rParams->block_size_log = rParams->dictionary_log + 2;
    rParams->divide_and_conquer = params->divideAndConquer;
    rParams->depth = params->searchDepth;
}

FL2LIB_API FL2_CCtx* FL2LIB_CALL FL2_createCCtx(void)
{
    return FL2_createCCtxMt(1);
}

FL2LIB_API FL2_CCtx* FL2LIB_CALL FL2_createCCtxMt(unsigned nbThreads)
{
    FL2_CCtx* cctx;

#ifndef FL2_SINGLETHREAD
    if (!nbThreads) {
        nbThreads = UTIL_countPhysicalCores();
        nbThreads += !nbThreads;
    }
    if (nbThreads > FL2_MAXTHREADS) {
        nbThreads = FL2_MAXTHREADS;
    }
#else
    nbThreads = 1;
#endif

    DEBUGLOG(3, "FL2_createCCtxMt : %u threads", nbThreads);

    cctx = malloc(sizeof(FL2_CCtx) + (nbThreads - 1) * sizeof(FL2_job));
    if (cctx == NULL)
        return NULL;

    cctx->jobCount = nbThreads;
    for (unsigned u = 0; u < nbThreads; ++u) {
        cctx->jobs[u].enc = NULL;
    }

    cctx->params.highCompression = 0;
    FL2_CCtx_setParameter(cctx, FL2_p_compressionLevel, FL2_CLEVEL_DEFAULT);
#ifndef NO_XXHASH
    cctx->params.doXXH = 1;
#endif
    cctx->params.omitProp = 0;

#ifdef RMF_REFERENCE
    cctx->params.rParams.use_ref_mf = 0;
#endif

    cctx->matchTable = NULL;

#ifndef FL2_SINGLETHREAD
    cctx->factory = FL2POOL_create(nbThreads - 1);
    if (nbThreads > 1 && cctx->factory == NULL) {
        FL2_freeCCtx(cctx);
        return NULL;
    }
#endif

    for (unsigned u = 0; u < nbThreads; ++u) {
        cctx->jobs[u].enc = FL2_lzma2Create();
        if (cctx->jobs[u].enc == NULL) {
            FL2_freeCCtx(cctx);
            return NULL;
        }
        cctx->jobs[u].cctx = cctx;
    }
    cctx->dictMax = 0;
    cctx->block_total = 0;

    return cctx;
}

FL2LIB_API void FL2LIB_CALL FL2_freeCCtx(FL2_CCtx* cctx)
{
    if (cctx == NULL) 
        return;

    DEBUGLOG(3, "FL2_freeCCtx : %u threads", cctx->jobCount);

    for (unsigned u = 0; u < cctx->jobCount; ++u) {
        FL2_lzma2Free(cctx->jobs[u].enc);
    }

#ifndef FL2_SINGLETHREAD
    FL2POOL_free(cctx->factory);
#endif

    RMF_freeMatchTable(cctx->matchTable);
    free(cctx);
}

FL2LIB_API unsigned FL2LIB_CALL FL2_CCtx_nbThreads(const FL2_CCtx* cctx)
{
    return cctx->jobCount;
}

/* FL2_buildRadixTable() : FL2POOL_function type */
static void FL2_buildRadixTable(void* const jobDescription, size_t n)
{
    const FL2_job* const job = (FL2_job*)jobDescription;
    FL2_CCtx* const cctx = job->cctx;

    RMF_buildTable(cctx->matchTable, n, 1, cctx->curBlock, NULL, NULL, 0, 0);
}

/* FL2_compressRadixChunk() : FL2POOL_function type */
static void FL2_compressRadixChunk(void* const jobDescription, size_t n)
{
    const FL2_job* const job = (FL2_job*)jobDescription;
    FL2_CCtx* const cctx = job->cctx;

    cctx->jobs[n].cSize = FL2_lzma2Encode(cctx->jobs[n].enc, cctx->matchTable, job->block, &cctx->params.cParams, NULL, NULL, 0, 0);
}

static int FL2_initEncoders(FL2_CCtx* const cctx)
{
    for(unsigned u = 0; u < cctx->jobCount; ++u) {
        if (FL2_lzma2HashAlloc(cctx->jobs[u].enc, &cctx->params.cParams) != 0)
            return 1;
    }
    return 0;
}

static size_t FL2_compressCurBlock(FL2_CCtx* const cctx, FL2_progressFn progress, void* opaque)
{
    size_t const encodeSize = (cctx->curBlock.end - cctx->curBlock.start);
    size_t init_done;
    U32 rmf_weight = ZSTD_highbit32((U32)cctx->curBlock.end);
    U32 depth_weight = 2 + (cctx->params.rParams.depth >= 12) + (cctx->params.rParams.depth >= 28);
    U32 enc_weight;
    int err = 0;
#ifndef FL2_SINGLETHREAD
    size_t mfThreads = cctx->curBlock.end / RMF_MIN_BYTES_PER_THREAD;
    size_t nbThreads = MIN(cctx->jobCount, encodeSize / MIN_BYTES_PER_THREAD);
    nbThreads += !nbThreads;
#else
    size_t mfThreads = 1;
    size_t nbThreads = 1;
#endif

    if (rmf_weight >= 20) {
        rmf_weight = depth_weight * (rmf_weight - 10) + (rmf_weight - 19) * 12;
        if (cctx->params.cParams.strategy == 0)
            enc_weight = 20;
        else if (cctx->params.cParams.strategy == 1)
            enc_weight = 50;
        else
            enc_weight = 60 + cctx->params.cParams.second_dict_bits + ZSTD_highbit32(cctx->params.cParams.fast_length) * 3U;
        rmf_weight = (rmf_weight << 4) / (rmf_weight + enc_weight);
        enc_weight = 16 - rmf_weight;
    }
    else {
        rmf_weight = 8;
        enc_weight = 8;
    }

    DEBUGLOG(5, "FL2_compressCurBlock : %u threads, %u start, %u bytes", (U32)nbThreads, (U32)cctx->curBlock.start, (U32)encodeSize);

    /* Free unsuitable match table before reallocating anything else */
    if (cctx->matchTable && !RMF_compatibleParameters(cctx->matchTable, &cctx->params.rParams, cctx->curBlock.end)) {
        RMF_freeMatchTable(cctx->matchTable);
        cctx->matchTable = NULL;
    }

    if(FL2_initEncoders(cctx) != 0) /* Create hash objects together, leaving the (large) match table last */
        return FL2_ERROR(memory_allocation);

    if (!cctx->matchTable) {
        cctx->matchTable = RMF_createMatchTable(&cctx->params.rParams, cctx->curBlock.end, cctx->jobCount);
        if (cctx->matchTable == NULL)
            return FL2_ERROR(memory_allocation);
    }
    else {
        DEBUGLOG(5, "Have compatible match table");
        RMF_applyParameters(cctx->matchTable, &cctx->params.rParams, cctx->curBlock.end);
    }

    {   size_t sliceStart = cctx->curBlock.start;
        size_t sliceSize = encodeSize / nbThreads;
        cctx->jobs[0].block.data = cctx->curBlock.data;
        cctx->jobs[0].block.start = sliceStart;
        cctx->jobs[0].block.end = sliceStart + sliceSize;

        for (size_t u = 1; u < nbThreads; ++u) {
            sliceStart += sliceSize;
            cctx->jobs[u].block.data = cctx->curBlock.data;
            cctx->jobs[u].block.start = sliceStart;
            cctx->jobs[u].block.end = sliceStart + sliceSize;
        }
        cctx->jobs[nbThreads - 1].block.end = cctx->curBlock.end;
    }

    /* update largest dict size used */
    cctx->dictMax = MAX(cctx->dictMax, cctx->curBlock.end);

    /* initialize to length 2 */
    init_done = RMF_initTable(cctx->matchTable, cctx->curBlock.data, cctx->curBlock.start, cctx->curBlock.end);

#ifndef FL2_SINGLETHREAD
    mfThreads = MIN(RMF_threadCount(cctx->matchTable), mfThreads);
    for (size_t u = 1; u < mfThreads; ++u) {
		FL2POOL_add(cctx->factory, FL2_buildRadixTable, &cctx->jobs[u], u);
    }
#endif

    err = RMF_buildTable(cctx->matchTable, 0, mfThreads > 1, cctx->curBlock, progress, opaque, rmf_weight, init_done);

#ifndef FL2_SINGLETHREAD

    FL2POOL_waitAll(cctx->factory);

    if (err)
        return FL2_ERROR(canceled);

#ifdef RMF_CHECK_INTEGRITY
    err = RMF_integrityCheck(cctx->matchTable, cctx->curBlock.data, cctx->curBlock.start, cctx->curBlock.end, cctx->params.rParams.depth);
    if (err)
        return FL2_ERROR(internal);
#endif

    for (size_t u = 1; u < nbThreads; ++u) {
		FL2POOL_add(cctx->factory, FL2_compressRadixChunk, &cctx->jobs[u], u);
    }

    cctx->jobs[0].cSize = FL2_lzma2Encode(cctx->jobs[0].enc, cctx->matchTable, cctx->jobs[0].block, &cctx->params.cParams, progress, opaque, (rmf_weight * encodeSize) >> 4, enc_weight * (U32)nbThreads);
    FL2POOL_waitAll(cctx->factory);

#else /* FL2_SINGLETHREAD */

    if (err)
        return FL2_ERROR(canceled);

#ifdef RMF_CHECK_INTEGRITY
    err = RMF_integrityCheck(cctx->matchTable, cctx->curBlock.data, cctx->curBlock.start, cctx->curBlock.end, cctx->params.rParams.depth);
    if (err)
        return FL2_ERROR(internal);
#endif
    cctx->jobs[0].cSize = FL2_lzma2Encode(cctx->jobs[0].enc, cctx->matchTable, cctx->jobs[0].block, &cctx->params.cParams, progress, opaque, (rmf_weight * encodeSize) >> 4, enc_weight);

#endif

    return nbThreads;
}

FL2LIB_API void FL2LIB_CALL FL2_beginFrame(FL2_CCtx* const cctx)
{
    cctx->dictMax = 0;
    cctx->block_total = 0;
}

static size_t FL2_compressBlock(FL2_CCtx* const cctx,
    const void* const src, size_t srcStart, size_t const srcEnd,
    void* const dst, size_t dstCapacity,
    FL2_writerFn const writeFn, void* const opaque,
    FL2_progressFn progress)
{
    BYTE* dstBuf = dst;
    size_t outSize = 0;
    size_t const dictionary_size = (size_t)1 << cctx->params.rParams.dictionary_log;
    size_t const block_overlap = OVERLAP_FROM_DICT_LOG(cctx->params.rParams.dictionary_log, cctx->params.rParams.overlap_fraction);

    if (srcStart >= srcEnd)
        return 0;
    cctx->curBlock.data = src;
    cctx->curBlock.start = srcStart;

    while (srcStart < srcEnd) {
        size_t nbThreads;

        cctx->curBlock.end = cctx->curBlock.start + MIN(srcEnd - srcStart, dictionary_size - cctx->curBlock.start);

        nbThreads = FL2_compressCurBlock(cctx, progress, opaque);
        if (FL2_isError(nbThreads))
            return nbThreads;

        for (size_t u = 0; u < nbThreads; ++u) {
            const BYTE* const outBuf = RMF_getTableAsOutputBuffer(cctx->matchTable, cctx->jobs[u].block.start);

            if (FL2_isError(cctx->jobs[u].cSize))
                return cctx->jobs[u].cSize;

            DEBUGLOG(5, "Write thread %u : %u bytes", (U32)u, (U32)cctx->jobs[u].cSize);

            if (writeFn == NULL && dstCapacity < cctx->jobs[u].cSize) {
                return FL2_ERROR(dstSize_tooSmall);
            }
            if (writeFn != NULL) {
                if(writeFn(outBuf, cctx->jobs[u].cSize, opaque))
                    return FL2_ERROR(write_failed);
                outSize += cctx->jobs[u].cSize;
            }
            else {
                memcpy(dstBuf, outBuf, cctx->jobs[u].cSize);
                dstBuf += cctx->jobs[u].cSize;
                dstCapacity -= cctx->jobs[u].cSize;
            }
        }
        srcStart += cctx->curBlock.end - cctx->curBlock.start;
        cctx->block_total += cctx->curBlock.end - cctx->curBlock.start;
        if (cctx->params.rParams.block_size_log && cctx->block_total + MIN(cctx->curBlock.end - block_overlap, srcEnd - srcStart) > ((U64)1 << cctx->params.rParams.block_size_log)) {
            /* periodically reset the dictionary for mt decompression */
            cctx->curBlock.start = 0;
            cctx->block_total = 0;
        }
        else {
            cctx->curBlock.start = block_overlap;
        }
        cctx->curBlock.data += cctx->curBlock.end - cctx->curBlock.start;
    }
    return (writeFn != NULL) ? outSize : dstBuf - (const BYTE*)dst;
}

static BYTE FL2_getProp(FL2_CCtx* cctx, size_t dictionary_size)
{
    return FL2_getDictSizeProp(dictionary_size)
#ifndef NO_XXHASH
        | (BYTE)((cctx->params.doXXH != 0) << FL2_PROP_HASH_BIT)
#endif
        ;
}

FL2LIB_API size_t FL2LIB_CALL FL2_compressCCtx(FL2_CCtx* cctx,
    void* dst, size_t dstCapacity,
    const void* src, size_t srcSize,
    int compressionLevel)
{
    BYTE* dstBuf = dst;
    BYTE* const end = dstBuf + dstCapacity;
    size_t cSize = 0;

    if (compressionLevel > 0)
        FL2_CCtx_setParameter(cctx, FL2_p_compressionLevel, compressionLevel);

    DEBUGLOG(4, "FL2_compressCCtx : level %u, %u src => %u avail", cctx->params.compressionLevel, (U32)srcSize, (U32)dstCapacity);

    if (dstCapacity < 2U - cctx->params.omitProp) /* empty LZMA2 stream is byte sequence {0, 0} */
        return FL2_ERROR(dstSize_tooSmall);

    FL2_beginFrame(cctx);

    dstBuf += !cctx->params.omitProp;
    cSize = FL2_compressBlock(cctx, src, 0, srcSize, dstBuf, end - dstBuf, NULL, NULL, NULL);
    if(!cctx->params.omitProp)
        dstBuf[-1] = FL2_getProp(cctx, cctx->dictMax);

    if (FL2_isError(cSize))
        return cSize;

    dstBuf += cSize;
    if(dstBuf >= end)
        return FL2_ERROR(dstSize_tooSmall);
    *dstBuf++ = LZMA2_END_MARKER;

#ifndef NO_XXHASH
    if (cctx->params.doXXH && !cctx->params.omitProp) {
        XXH32_canonical_t canonical;
        DEBUGLOG(5, "Writing hash");
        if(end - dstBuf < XXHASH_SIZEOF)
            return FL2_ERROR(dstSize_tooSmall);
        XXH32_canonicalFromHash(&canonical, XXH32(src, srcSize, 0));
        memcpy(dstBuf, &canonical, XXHASH_SIZEOF);
        dstBuf += XXHASH_SIZEOF;
    }
#endif
    return dstBuf - (BYTE*)dst;
}

FL2LIB_API size_t FL2LIB_CALL FL2_blockOverlap(const FL2_CCtx* cctx)
{
	return OVERLAP_FROM_DICT_LOG(cctx->params.rParams.dictionary_log, cctx->params.rParams.overlap_fraction);
}

FL2LIB_API void FL2LIB_CALL FL2_shiftBlock(FL2_CCtx* cctx, FL2_blockBuffer *block)
{
    FL2_shiftBlock_switch(cctx, block, NULL);
}

FL2LIB_API void FL2LIB_CALL FL2_shiftBlock_switch(FL2_CCtx* cctx, FL2_blockBuffer *block, unsigned char *dst)
{
    size_t const block_overlap = OVERLAP_FROM_DICT_LOG(cctx->params.rParams.dictionary_log, cctx->params.rParams.overlap_fraction);

	if (block_overlap == 0) {
		block->start = 0;
		block->end = 0;
	}
	else if (block->end > block_overlap) {
        size_t const from = (block->end - block_overlap) & ALIGNMENT_MASK;
        size_t overlap = block->end - from;

        cctx->block_total += block->end - block->start;
        if (cctx->params.rParams.block_size_log && cctx->block_total + from > ((U64)1 << cctx->params.rParams.block_size_log)) {
            /* periodically reset the dictionary for mt decompression */
            overlap = 0;
            cctx->block_total = 0;
        }
        else if (overlap <= from || dst != NULL) {
            DEBUGLOG(5, "Copy overlap data : %u bytes", (U32)overlap);
            memcpy(dst ? dst : block->data, block->data + from, overlap);
        }
		else if (from != 0) {
            DEBUGLOG(5, "Move overlap data : %u bytes", (U32)overlap);
            memmove(block->data, block->data + from, overlap);
        }
        block->start = overlap;
        block->end = overlap;
    }
    else {
        block->start = block->end;
    }
}

FL2LIB_API size_t FL2LIB_CALL FL2_compressCCtxBlock(FL2_CCtx* cctx,
    void* dst, size_t dstCapacity,
    const FL2_blockBuffer *block,
    FL2_progressFn progress, void* opaque)
{
    return FL2_compressBlock(cctx, block->data, block->start, block->end, dst, dstCapacity, NULL, opaque, progress);
}

FL2LIB_API size_t FL2LIB_CALL FL2_endFrame(FL2_CCtx* ctx,
    void* dst, size_t dstCapacity)
{
    if (!dstCapacity)
        return FL2_ERROR(dstSize_tooSmall);
    *(BYTE*)dst = LZMA2_END_MARKER;
    return 1;
}

FL2LIB_API size_t FL2LIB_CALL FL2_compressCCtxBlock_toFn(FL2_CCtx* cctx,
    FL2_writerFn writeFn, void* opaque,
    const FL2_blockBuffer *block,
    FL2_progressFn progress)
{
    return FL2_compressBlock(cctx, block->data, block->start, block->end, NULL, 0, writeFn, opaque, progress);
}

FL2LIB_API size_t FL2LIB_CALL FL2_endFrame_toFn(FL2_CCtx* ctx,
    FL2_writerFn writeFn, void* opaque)
{
    BYTE c = LZMA2_END_MARKER;
    if(writeFn(&c, 1, opaque))
        return FL2_ERROR(write_failed);
    return 1;
}

FL2LIB_API size_t FL2LIB_CALL FL2_compressMt(void* dst, size_t dstCapacity,
    const void* src, size_t srcSize,
    int compressionLevel,
    unsigned nbThreads)
{
    size_t cSize;
    FL2_CCtx* const cctx = FL2_createCCtxMt(nbThreads);
    if (cctx == NULL)
        return FL2_ERROR(memory_allocation);

    cSize = FL2_compressCCtx(cctx, dst, dstCapacity, src, srcSize, compressionLevel);

    FL2_freeCCtx(cctx);
    return cSize;
}

FL2LIB_API size_t FL2LIB_CALL FL2_compress(void* dst, size_t dstCapacity,
    const void* src, size_t srcSize,
    int compressionLevel)
{
    return FL2_compressMt(dst, dstCapacity, src, srcSize, compressionLevel, 1);
}

FL2LIB_API BYTE FL2LIB_CALL FL2_dictSizeProp(FL2_CCtx* cctx)
{
    return FL2_getDictSizeProp(cctx->dictMax ? cctx->dictMax : (size_t)1 << cctx->params.rParams.dictionary_log);
}

#define CLAMPCHECK(val,min,max) {            \
    if (((val)<(min)) | ((val)>(max))) {     \
        return FL2_ERROR(parameter_outOfBound);  \
}   }

FL2LIB_API size_t FL2LIB_CALL FL2_CCtx_setParameter(FL2_CCtx* cctx, FL2_cParameter param, unsigned value)
{
    switch (param)
    {
    case FL2_p_compressionLevel:
        if (value > 0) { /* 0 : does not change current level */
            if (cctx->params.highCompression) {
                if ((int)value > FL2_MAX_HIGH_CLEVEL) value = FL2_MAX_HIGH_CLEVEL;
                FL2_fillParameters(cctx, &FL2_highCParameters[value]);
            }
            else {
                if ((int)value > FL2_MAX_CLEVEL) value = FL2_MAX_CLEVEL;
                FL2_fillParameters(cctx, &FL2_defaultCParameters[value]);
            }
            cctx->params.compressionLevel = value;
        }
        return cctx->params.compressionLevel;

    case FL2_p_highCompression:
        if ((int)value >= 0) { /* < 0 : does not change highCompression */
            cctx->params.highCompression = value != 0;
            FL2_CCtx_setParameter(cctx, FL2_p_compressionLevel, cctx->params.compressionLevel);
        }
        return cctx->params.highCompression;

    case FL2_p_7zLevel:
        if (value > 0) { /* 0 : does not change current level */
            if ((int)value > FL2_MAX_7Z_CLEVEL) value = FL2_MAX_7Z_CLEVEL;
            FL2_fillParameters(cctx, &FL2_7zCParameters[value]);
            cctx->params.compressionLevel = value;
        }
        return cctx->params.compressionLevel;

    case FL2_p_dictionaryLog:
        if (value) {  /* 0 : does not change current dictionaryLog */
            CLAMPCHECK(value, FL2_DICTLOG_MIN, FL2_DICTLOG_MAX);
            cctx->params.rParams.dictionary_log = value;
        }
        return cctx->params.rParams.dictionary_log;

    case FL2_p_overlapFraction:
        if ((int)value >= 0) {  /* < 0 : does not change current overlapFraction */
            CLAMPCHECK(value, FL2_BLOCK_OVERLAP_MIN, FL2_BLOCK_OVERLAP_MAX);
            cctx->params.rParams.overlap_fraction = value;
        }
        return cctx->params.rParams.overlap_fraction;

    case FL2_p_blockSize:
        if ((int)value >= 0) {  /* < 0 : does not change current overlapFraction */
            CLAMPCHECK(value, FL2_BLOCK_LOG_MIN, FL2_BLOCK_LOG_MAX);
            cctx->params.rParams.block_size_log = value;
        }
        return cctx->params.rParams.block_size_log;

    case FL2_p_bufferLog:
        if (value) {  /* 0 : does not change current bufferLog */
            CLAMPCHECK(value, FL2_BUFFER_SIZE_LOG_MIN, FL2_BUFFER_SIZE_LOG_MAX);
            cctx->params.rParams.match_buffer_log = value;
        }
        return cctx->params.rParams.match_buffer_log;

    case FL2_p_chainLog:
        if (value) { /* 0 : does not change current chainLog */
            CLAMPCHECK(value, FL2_CHAINLOG_MIN, FL2_CHAINLOG_MAX);
            cctx->params.cParams.second_dict_bits = value;
        }
        return cctx->params.cParams.second_dict_bits;

    case FL2_p_searchLog:
        if ((int)value >= 0) { /* < 0 : does not change current searchLog */
            CLAMPCHECK(value, FL2_SEARCHLOG_MIN, FL2_SEARCHLOG_MAX);
            cctx->params.cParams.match_cycles = 1U << value;
        }
        return value;

    case FL2_p_literalCtxBits:
        if ((int)value >= 0) { /* < 0 : does not change current lc */
            CLAMPCHECK(value, FL2_LC_MIN, FL2_LC_MAX);
            cctx->params.cParams.lc = value;
        }
        return cctx->params.cParams.lc;

    case FL2_p_literalPosBits:
        if ((int)value >= 0) { /* < 0 : does not change current lp */
            CLAMPCHECK(value, FL2_LP_MIN, FL2_LP_MAX);
            cctx->params.cParams.lp = value;
        }
        return cctx->params.cParams.lp;

    case FL2_p_posBits:
        if ((int)value >= 0) { /* < 0 : does not change current pb */
            CLAMPCHECK(value, FL2_PB_MIN, FL2_PB_MAX);
            cctx->params.cParams.pb = value;
        }
        return cctx->params.cParams.pb;

    case FL2_p_searchDepth:
        if (value) { /* 0 : does not change current depth */
            CLAMPCHECK(value, FL2_SEARCH_DEPTH_MIN, FL2_SEARCH_DEPTH_MAX);
            cctx->params.rParams.depth = value;
        }
        return cctx->params.rParams.depth;

    case FL2_p_fastLength:
        if (value) { /* 0 : does not change current fast_length */
            CLAMPCHECK(value, FL2_FASTLENGTH_MIN, FL2_FASTLENGTH_MAX);
            cctx->params.cParams.fast_length = value;
        }
        return cctx->params.cParams.fast_length;

    case FL2_p_divideAndConquer:
        if ((int)value >= 0) { /* < 0 : does not change current divide_and_conquer */
            cctx->params.rParams.divide_and_conquer = value;
        }
        return cctx->params.rParams.divide_and_conquer;

    case FL2_p_strategy:
        if ((int)value >= 0) { /* < 0 : does not change current strategy */
            CLAMPCHECK(value, (unsigned)FL2_fast, (unsigned)FL2_ultra);
            cctx->params.cParams.strategy = (FL2_strategy)value;
        }
        return (size_t)cctx->params.cParams.strategy;

#ifndef NO_XXHASH
    case FL2_p_doXXHash:
        if ((int)value >= 0) { /* < 0 : does not change doXXHash */
            cctx->params.doXXH = value != 0;
        }
        return cctx->params.doXXH;
#endif

    case FL2_p_omitProperties:
        if ((int)value >= 0) { /* < 0 : does not change omitProp */
            cctx->params.omitProp = value != 0;
        }
        return cctx->params.omitProp;
#ifdef RMF_REFERENCE
    case FL2_p_useReferenceMF:
        if ((int)value >= 0) { /* < 0 : does not change useRefMF */
            cctx->params.rParams.use_ref_mf = value != 0;
        }
        return cctx->params.rParams.use_ref_mf;
#endif
    default: return FL2_ERROR(parameter_unsupported);
    }
}

FL2LIB_API FL2_CStream* FL2LIB_CALL FL2_createCStream(void)
{
    FL2_CCtx* const cctx = FL2_createCCtx();
    FL2_CStream* const fcs = malloc(sizeof(FL2_CStream));

    DEBUGLOG(3, "FL2_createCStream");

    if (cctx == NULL || fcs == NULL) {
        free(cctx);
        free(fcs);
        return NULL;
    }
    fcs->cctx = cctx;
    fcs->inBuff.bufSize = 0;
    fcs->inBuff.data = NULL;
    fcs->inBuff.start = 0;
    fcs->inBuff.end = 0;
#ifndef NO_XXHASH
    fcs->xxh = NULL;
#endif
    fcs->out_thread = 0;
    fcs->thread_count = 0;
    fcs->out_pos = 0;
    fcs->hash_pos = 0;
    fcs->end_marked = 0;
    fcs->wrote_prop = 0;
    return fcs;
}

FL2LIB_API size_t FL2LIB_CALL FL2_freeCStream(FL2_CStream* fcs)
{
    if (fcs == NULL)
        return 0;

    DEBUGLOG(3, "FL2_freeCStream");

    free(fcs->inBuff.data);
#ifndef NO_XXHASH
    XXH32_freeState(fcs->xxh);
#endif
    FL2_freeCCtx(fcs->cctx);
    free(fcs);
    return 0;
}

FL2LIB_API size_t FL2LIB_CALL FL2_initCStream(FL2_CStream* fcs, int compressionLevel)
{
    DEBUGLOG(4, "FL2_initCStream level %d", compressionLevel);

    fcs->inBuff.start = 0;
    fcs->inBuff.end = 0;
    fcs->out_thread = 0;
    fcs->thread_count = 0;
    fcs->out_pos = 0;
    fcs->hash_pos = 0;
    fcs->end_marked = 0;
    fcs->wrote_prop = 0;

    FL2_CCtx_setParameter(fcs->cctx, FL2_p_compressionLevel, compressionLevel);

#ifndef NO_XXHASH
    if (fcs->cctx->params.doXXH && !fcs->cctx->params.omitProp) {
        if (fcs->xxh == NULL) {
            fcs->xxh = XXH32_createState();
            if (fcs->xxh == NULL)
                return FL2_ERROR(memory_allocation);
        }
        XXH32_reset(fcs->xxh, 0);
    }
#endif

    FL2_beginFrame(fcs->cctx);
    return 0;
}

static size_t FL2_compressStream_internal(FL2_CStream* const fcs,
    FL2_outBuffer* const output, int const ending)
{
    FL2_CCtx* const cctx = fcs->cctx;

    if (output->pos >= output->size)
        return 0;

    if (fcs->out_thread == fcs->thread_count) {
        if (fcs->inBuff.start < fcs->inBuff.end) {
#ifndef NO_XXHASH
            if (cctx->params.doXXH && !cctx->params.omitProp) {
                XXH32_update(fcs->xxh, fcs->inBuff.data + fcs->inBuff.start, fcs->inBuff.end - fcs->inBuff.start);
            }
#endif
            cctx->curBlock.data = fcs->inBuff.data;
            cctx->curBlock.start = fcs->inBuff.start;
            cctx->curBlock.end = fcs->inBuff.end;

            fcs->out_thread = 0;
            fcs->thread_count = FL2_compressCurBlock(cctx, NULL, NULL);
            if (FL2_isError(fcs->thread_count))
                return fcs->thread_count;

            fcs->inBuff.start = fcs->inBuff.end;
        }
        if (!fcs->wrote_prop && !cctx->params.omitProp) {
            size_t dictionary_size = ending ? cctx->dictMax : (size_t)1 << cctx->params.rParams.dictionary_log;
            ((BYTE*)output->dst)[output->pos] = FL2_getProp(cctx, dictionary_size);
            DEBUGLOG(4, "Writing property byte : 0x%X", ((BYTE*)output->dst)[output->pos]);
            ++output->pos;
            fcs->wrote_prop = 1;
        }
    }
    for (; fcs->out_thread < fcs->thread_count; ++fcs->out_thread) {
        const BYTE* const outBuf = RMF_getTableAsOutputBuffer(cctx->matchTable, cctx->jobs[fcs->out_thread].block.start) + fcs->out_pos;
        BYTE* const dstBuf = (BYTE*)output->dst + output->pos;
        size_t const dstCapacity = output->size - output->pos;
        size_t to_write = cctx->jobs[fcs->out_thread].cSize;

        if (FL2_isError(to_write))
            return to_write;

        to_write = MIN(to_write - fcs->out_pos, dstCapacity);

        DEBUGLOG(5, "CStream : writing %u bytes", (U32)to_write);

        memcpy(dstBuf, outBuf, to_write);
        fcs->out_pos += to_write;
        output->pos += to_write;

        if (fcs->out_pos < cctx->jobs[fcs->out_thread].cSize)
            break;

        fcs->out_pos = 0;
    }
    return 0;
}

static size_t FL2_remainingOutputSize(FL2_CStream* const fcs)
{
    FL2_CCtx* const cctx = fcs->cctx;
    size_t pos = fcs->out_pos;
    size_t total = 0;

    if (FL2_isError(fcs->thread_count))
        return fcs->thread_count;

    for (size_t u = fcs->out_thread; u < fcs->thread_count; ++u) {
        size_t to_write = cctx->jobs[u].cSize;

        if (FL2_isError(to_write))
            return to_write;
        total += to_write - pos;
        pos = 0;
    }
    return total;
}

FL2LIB_API size_t FL2LIB_CALL FL2_compressStream(FL2_CStream* fcs, FL2_outBuffer* output, FL2_inBuffer* input)
{
    FL2_blockBuffer* const inBuff = &fcs->inBuff;
    FL2_CCtx* const cctx = fcs->cctx;
    size_t block_overlap = OVERLAP_FROM_DICT_LOG(cctx->params.rParams.dictionary_log, cctx->params.rParams.overlap_fraction);

    if (FL2_isError(fcs->thread_count))
        return fcs->thread_count;

    if (output->pos < output->size) while (input->pos < input->size) {
        /* read input and/or write output until a buffer is full */
        if (inBuff->data == NULL) {
            inBuff->bufSize = (size_t)1 << cctx->params.rParams.dictionary_log;

            DEBUGLOG(3, "Allocating input buffer : %u bytes", (U32)inBuff->bufSize);

            inBuff->data = malloc(inBuff->bufSize);

            if (inBuff->data == NULL)
                return FL2_ERROR(memory_allocation);

            inBuff->start = 0;
            inBuff->end = 0;
        }
        if (inBuff->start > block_overlap && input->pos < input->size) {
            FL2_shiftBlock(fcs->cctx, inBuff);
        }
        if (fcs->out_thread == fcs->thread_count) {
            /* no compressed output to write, so read */
            size_t const toRead = MIN(input->size - input->pos, inBuff->bufSize - inBuff->end);

            DEBUGLOG(5, "CStream : reading %u bytes", (U32)toRead);

            memcpy(inBuff->data + inBuff->end, (char*)input->src + input->pos, toRead);
            input->pos += toRead;
            inBuff->end += toRead;
        }
        if (inBuff->end == inBuff->bufSize || fcs->out_thread < fcs->thread_count) {
            CHECK_F(FL2_compressStream_internal(fcs, output, 0));
        }
        /* compressed output remains, so output buffer is full */
        if (fcs->out_thread < fcs->thread_count)
            break;
    }
    return (inBuff->data == NULL) ? (size_t)1 << cctx->params.rParams.dictionary_log : inBuff->bufSize - inBuff->end;
}

static size_t FL2_flushStream_internal(FL2_CStream* fcs, FL2_outBuffer* output, int ending)
{
    if (FL2_isError(fcs->thread_count))
        return fcs->thread_count;

    DEBUGLOG(4, "FL2_flushStream_internal : %u to compress, %u to write",
        (U32)(fcs->inBuff.end - fcs->inBuff.start),
        (U32)FL2_remainingOutputSize(fcs));

    CHECK_F(FL2_compressStream_internal(fcs, output, ending));

    return FL2_remainingOutputSize(fcs);
}

FL2LIB_API size_t FL2LIB_CALL FL2_flushStream(FL2_CStream* fcs, FL2_outBuffer* output)
{
    return FL2_flushStream_internal(fcs, output, 0);
}

FL2LIB_API size_t FL2LIB_CALL FL2_endStream(FL2_CStream* fcs, FL2_outBuffer* output)
{
    {   size_t cSize = FL2_flushStream_internal(fcs, output, 1);
        if (cSize != 0)
            return cSize;
    }

    if(!fcs->end_marked) {
        if (output->pos >= output->size)
            return 1;
        DEBUGLOG(4, "Writing end marker");
        ((BYTE*)output->dst)[output->pos] = LZMA2_END_MARKER;
        ++output->pos;
        fcs->end_marked = 1;
    }

#ifndef NO_XXHASH
    if (fcs->cctx->params.doXXH && !fcs->cctx->params.omitProp && fcs->hash_pos < XXHASH_SIZEOF) {
        size_t const to_write = MIN(output->size - output->pos, XXHASH_SIZEOF - fcs->hash_pos);
        XXH32_canonical_t canonical;

        if (output->pos >= output->size)
            return 1;

        XXH32_canonicalFromHash(&canonical, XXH32_digest(fcs->xxh));
        DEBUGLOG(4, "Writing XXH32 : %u bytes", (U32)to_write);
        memcpy((BYTE*)output->dst + output->pos, canonical.digest + fcs->hash_pos, to_write);
        output->pos += to_write;
        fcs->hash_pos += to_write;
        return fcs->hash_pos < XXHASH_SIZEOF;
    }
#endif
    return 0;
}

FL2LIB_API size_t FL2LIB_CALL FL2_CStream_setParameter(FL2_CStream* fcs, FL2_cParameter param, unsigned value)
{
    if (fcs->inBuff.start < fcs->inBuff.end)
        return FL2_ERROR(stage_wrong);
    return FL2_CCtx_setParameter(fcs->cctx, param, value);
}


size_t FL2_memoryUsage_internal(unsigned const dictionaryLog, unsigned const bufferLog, unsigned const searchDepth,
    unsigned chainLog, FL2_strategy strategy,
    unsigned nbThreads)
{
    size_t size = RMF_memoryUsage(dictionaryLog, bufferLog, searchDepth, nbThreads);
    return size + FL2_lzma2MemoryUsage(chainLog, strategy, nbThreads);
}

FL2LIB_API size_t FL2LIB_CALL FL2_estimateCCtxSize(int compressionLevel, unsigned nbThreads)
{
    return FL2_memoryUsage_internal(FL2_defaultCParameters[compressionLevel].dictionaryLog,
        FL2_defaultCParameters[compressionLevel].bufferLog,
        FL2_defaultCParameters[compressionLevel].searchDepth,
        FL2_defaultCParameters[compressionLevel].chainLog,
        FL2_defaultCParameters[compressionLevel].strategy,
        nbThreads);
}

FL2LIB_API size_t FL2LIB_CALL FL2_estimateCCtxSize_usingCCtx(const FL2_CCtx * cctx)
{
    return FL2_memoryUsage_internal(cctx->params.rParams.dictionary_log,
        cctx->params.rParams.match_buffer_log,
        cctx->params.rParams.depth,
        cctx->params.cParams.second_dict_bits,
        cctx->params.cParams.strategy,
        cctx->jobCount);
}

FL2LIB_API size_t FL2LIB_CALL FL2_estimateCStreamSize(int compressionLevel, unsigned nbThreads)
{
    return FL2_estimateCCtxSize(compressionLevel, nbThreads)
        + ((size_t)1 << FL2_defaultCParameters[compressionLevel].dictionaryLog);
}

FL2LIB_API size_t FL2LIB_CALL FL2_estimateCStreamSize_usingCCtx(const FL2_CStream* fcs)
{
    return FL2_estimateCCtxSize_usingCCtx(fcs->cctx)
        + ((size_t)1 << fcs->cctx->params.rParams.dictionary_log);
}
