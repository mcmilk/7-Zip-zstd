/*
 * Copyright (c) 2018, Conor McCarthy
 * All rights reserved.
 * Parts based on zstd_compress_internal.h copyright Yann Collet
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

#ifndef FL2_COMPRESS_H
#define FL2_COMPRESS_H

/*-*************************************
*  Dependencies
***************************************/
#include "mem.h"
#include "data_block.h"
#include "radix_internal.h"
#include "lzma2_enc.h"
#include "fast-lzma2.h"
#include "fl2threading.h"
#include "fl2pool.h"
#ifndef NO_XXHASH
#  include "xxhash.h"
#endif

#if defined (__cplusplus)
extern "C" {
#endif

typedef struct {
    unsigned dictionaryLog;    /* largest match distance : larger == more compression, more memory needed during decompression; >= 27 == more memory, slower */
    unsigned overlapFraction;  /* overlap between consecutive blocks in 1/16 units: larger == more compression, slower */
    unsigned chainLog;         /* fully searched segment : larger == more compression, slower, more memory; hybrid mode only (ultra) */
    unsigned searchLog;        /* nb of searches : larger == more compression, slower; hybrid mode only (ultra) */
    unsigned searchDepth;      /* maximum depth for resolving string matches : larger == more compression, slower; >= 64 == more memory, slower */
    unsigned fastLength;       /* acceptable match size for parser, not less than searchDepth : larger == more compression, slower; fast bytes parameter from 7-zip */
    unsigned divideAndConquer; /* split long chains of 2-byte matches into shorter chains with a small overlap : faster, somewhat less compression; enabled by default */
    unsigned bufferLog;        /* buffer size for processing match chains is (dictionaryLog - bufferLog) : when divideAndConquer enabled, affects compression; */
                               /* when divideAndConquer disabled, affects speed in a hardware-dependent manner */
    FL2_strategy strategy;     /* encoder strategy : fast, optimized or ultra (hybrid) */
} FL2_compressionParameters;

/*-*************************************
*  Context memory management
***************************************/

typedef struct {
    FL2_lzma2Parameters cParams;
    RMF_parameters rParams;
    unsigned compressionLevel;
    BYTE highCompression;
#ifndef NO_XXHASH
    BYTE doXXH;
#endif
    BYTE omitProp;
} FL2_CCtx_params;

typedef struct {
    FL2_CCtx* cctx;
    FL2_lzmaEncoderCtx* enc;
    FL2_dataBlock block;
    size_t cSize;
} FL2_job;

struct FL2_CCtx_s {
    FL2_CCtx_params params;
#ifndef FL2_SINGLETHREAD
    FL2POOL_ctx* factory;
#endif
    FL2_dataBlock curBlock;
    size_t dictMax;
    U64 block_total;
    FL2_matchTable* matchTable;
    unsigned jobCount;
    FL2_job jobs[1];
};

struct FL2_CStream_s {
    FL2_CCtx* cctx;
    FL2_blockBuffer inBuff;
#ifndef NO_XXHASH
    XXH32_state_t *xxh;
#endif
    size_t thread_count;
    size_t out_thread;
    size_t out_pos;
    size_t hash_pos;
    BYTE end_marked;
    BYTE wrote_prop;
};

#if defined (__cplusplus)
}
#endif


#endif /* FL2_COMPRESS_H */
