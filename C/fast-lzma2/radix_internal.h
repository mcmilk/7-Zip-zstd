/*
* Copyright (c) 2018, Conor McCarthy
* All rights reserved.
*
* This source code is licensed under both the BSD-style license (found in the
* LICENSE file in the root directory of this source tree) and the GPLv2 (found
* in the COPYING file in the root directory of this source tree).
* You may select, at your option, one of the above-listed licenses.
*/

#ifndef RADIX_INTERNAL_H
#define RADIX_INTERNAL_H

#include "atomic.h"
#include "radix_mf.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define DICTIONARY_LOG_MIN 12U
#define DICTIONARY_LOG_MAX_64 30U
#define DICTIONARY_LOG_MAX_32 27U
#define DEFAULT_BUFFER_LOG 8U
#define DEFAULT_BLOCK_OVERLAP 2U
#define DEFAULT_SEARCH_DEPTH 32U
#define DEFAULT_DIVIDEANDCONQUER 1
#define MAX_REPEAT 32
#define RADIX16_TABLE_SIZE (1UL << 16)
#define RADIX8_TABLE_SIZE (1UL << 8)
#define STACK_SIZE (RADIX16_TABLE_SIZE * 3)
#define MAX_BRUTE_FORCE_LIST_SIZE 5
#define BUFFER_LINK_MASK 0xFFFFFFU
#define MATCH_BUFFER_OVERLAP 6
#define BITPACK_MAX_LENGTH 63UL
#define STRUCTURED_MAX_LENGTH 255UL

#define RADIX_LINK_BITS 26
#define RADIX_LINK_MASK ((1UL << RADIX_LINK_BITS) - 1)
#define RADIX_NULL_LINK 0xFFFFFFFFUL

#define UNIT_BITS 2
#define UNIT_MASK ((1UL << UNIT_BITS) - 1)

typedef struct
{
    U32 head;
    U32 count;
} RMF_tableHead;

union src_data_u {
	BYTE chars[4];
	U32 u32;
};

typedef struct
{
    U32 from;
	union src_data_u src;
    U32 next;
} RMF_buildMatch;

typedef struct
{
    U32 prev_index;
    U32 list_count;
} RMF_listTail;

typedef struct
{
    U32 links[1 << UNIT_BITS];
    BYTE lengths[1 << UNIT_BITS];
} RMF_unit;

typedef struct
{
    unsigned max_len;
    U32* table;
    size_t match_buffer_size;
    size_t match_buffer_limit;
    RMF_listTail tails_8[RADIX8_TABLE_SIZE];
    RMF_tableHead stack[STACK_SIZE];
    RMF_listTail tails_16[RADIX16_TABLE_SIZE];
    RMF_buildMatch match_buffer[1];
} RMF_builder;

struct FL2_matchTable_s
{
    FL2_atomic st_index;
    long end_index;
    int isStruct;
    int allocStruct;
    unsigned thread_count;
    RMF_parameters params;
    RMF_builder** builders;
    U32 stack[RADIX16_TABLE_SIZE];
    RMF_tableHead list_heads[RADIX16_TABLE_SIZE];
    U32 table[1];
};

size_t RMF_bitpackInit(struct FL2_matchTable_s* const tbl, const void* data, size_t const start, size_t const end);
size_t RMF_structuredInit(struct FL2_matchTable_s* const tbl, const void* data, size_t const start, size_t const end);
int RMF_bitpackBuildTable(struct FL2_matchTable_s* const tbl,
	size_t const job,
    unsigned const multi_thread,
    FL2_dataBlock const block,
    FL2_progressFn progress, void* opaque, U32 weight, size_t init_done);
int RMF_structuredBuildTable(struct FL2_matchTable_s* const tbl,
	size_t const job,
    unsigned const multi_thread,
    FL2_dataBlock const block,
    FL2_progressFn progress, void* opaque, U32 weight, size_t init_done);
void RMF_recurseListChunk(RMF_builder* const tbl,
    const BYTE* const data_block,
    size_t const block_start,
    BYTE const depth,
    BYTE const max_depth,
    U32 const list_count,
    size_t const stack_base);
int RMF_bitpackIntegrityCheck(const struct FL2_matchTable_s* const tbl, const BYTE* const data, size_t index, size_t const end, unsigned const max_depth);
int RMF_structuredIntegrityCheck(const struct FL2_matchTable_s* const tbl, const BYTE* const data, size_t index, size_t const end, unsigned const max_depth);
void RMF_bitpackLimitLengths(struct FL2_matchTable_s* const tbl, size_t const index);
void RMF_structuredLimitLengths(struct FL2_matchTable_s* const tbl, size_t const index);
BYTE* RMF_bitpackAsOutputBuffer(struct FL2_matchTable_s* const tbl, size_t const index);
BYTE* RMF_structuredAsOutputBuffer(struct FL2_matchTable_s* const tbl, size_t const index);
size_t RMF_bitpackGetMatch(const struct FL2_matchTable_s* const tbl,
    const BYTE* const data,
    size_t const index,
    size_t const limit,
    unsigned const max_depth,
    size_t* const offset_ptr);
size_t RMF_structuredGetMatch(const struct FL2_matchTable_s* const tbl,
    const BYTE* const data,
    size_t const index,
    size_t const limit,
    unsigned const max_depth,
    size_t* const offset_ptr);

#if defined (__cplusplus)
}
#endif

#endif /* RADIX_INTERNAL_H */