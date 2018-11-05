/*
* Copyright (c) 2018, Conor McCarthy
* All rights reserved.
*
* This source code is licensed under both the BSD-style license (found in the
* LICENSE file in the root directory of this source tree) and the GPLv2 (found
* in the COPYING file in the root directory of this source tree).
* You may select, at your option, one of the above-listed licenses.
*/

#ifndef RADIX_MF_H
#define RADIX_MF_H

#include "fast-lzma2.h"
#include "data_block.h"

#if defined (__cplusplus)
extern "C" {
#endif

typedef struct FL2_matchTable_s FL2_matchTable;

#define OVERLAP_FROM_DICT_LOG(d, o) (((size_t)1 << ((d) - 4)) * (o))

#define RMF_MIN_BYTES_PER_THREAD 1024

typedef struct
{
    unsigned dictionary_log;
    unsigned match_buffer_log;
    unsigned overlap_fraction;
    unsigned block_size_log;
    unsigned divide_and_conquer;
    unsigned depth;
#ifdef RMF_REFERENCE
    unsigned use_ref_mf;
#endif
} RMF_parameters;

FL2_matchTable* RMF_createMatchTable(const RMF_parameters* const params, size_t const dict_reduce, unsigned const thread_count);
void RMF_freeMatchTable(FL2_matchTable* const tbl);
BYTE RMF_compatibleParameters(const FL2_matchTable* const tbl, const RMF_parameters* const params, size_t const dict_reduce);
size_t RMF_applyParameters(FL2_matchTable* const tbl, const RMF_parameters* const params, size_t const dict_reduce);
size_t RMF_threadCount(const FL2_matchTable * const tbl);
size_t RMF_initTable(FL2_matchTable* const tbl, const void* const data, size_t const start, size_t const end);
int RMF_buildTable(FL2_matchTable* const tbl,
	size_t const job,
    unsigned const multi_thread,
    FL2_dataBlock const block,
    FL2_progressFn progress, void* opaque, U32 weight, size_t init_done);
int RMF_integrityCheck(const FL2_matchTable* const tbl, const BYTE* const data, size_t const index, size_t const end, unsigned const max_depth);
void RMF_limitLengths(FL2_matchTable* const tbl, size_t const index);
BYTE* RMF_getTableAsOutputBuffer(FL2_matchTable* const tbl, size_t const index);
size_t RMF_memoryUsage(unsigned const dict_log, unsigned const buffer_log, unsigned const depth, unsigned thread_count);

#if defined (__cplusplus)
}
#endif

#endif /* RADIX_MF_H */